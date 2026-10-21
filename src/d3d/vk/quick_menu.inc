// Only the carousel crosses from CPU to GPU. The game frame stays on the GPU,
// with the usual frame fences and presentation cadence in both pause modes.
void VulkanBackend::QuickMenuBackground(Image& dst) {
    // Fixed small scratch image: no resize waits, frame readback, or held
    // screenshot. Each paused frame starts from that frame's display copy.
    if (!quick_background_.image &&
        !CreateImage(quick_background_, kColorFormat, 640, 360, 1, 1, false, false,
                     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)) return;
    const uint64_t vs = 0xF113000000000001ull, ps = 0xF113000000000003ull;
    Module(ps, [] {
        return std::string("Texture2D tex0 : register(t0);\nSamplerState smp0 : register(s0);\n"
                           "struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
                           "float4 main(VO i) : SV_Target {\n"
                           "float2 d = float2(2.0 / 640.0, 2.0 / 360.0);\n"
                           "float3 c = tex0.Sample(smp0, i.uv).rgb * 4;\n"
                           "c += (tex0.Sample(smp0, i.uv + float2(d.x, 0)).rgb +\n"
                           "tex0.Sample(smp0, i.uv - float2(d.x, 0)).rgb +\n"
                           "tex0.Sample(smp0, i.uv + float2(0, d.y)).rgb +\n"
                           "tex0.Sample(smp0, i.uv - float2(0, d.y)).rgb) * 2;\n"
                           "c += tex0.Sample(smp0, i.uv + d).rgb + tex0.Sample(smp0, i.uv - d).rgb +\n"
                           "tex0.Sample(smp0, i.uv + float2(d.x, -d.y)).rgb +\n"
                           "tex0.Sample(smp0, i.uv + float2(-d.x, d.y)).rgb;\n"
                           "return float4(c * (0.70 / 16.0), 1); }\n");
    }, false);
    PipeKey k{};
    k.vs = vs; k.ps = ps; k.topo = 2; k.wmask = 0xF; k.has_rt = 1; k.samples = 1;
    VkPipeline pipe = Pipeline(k);
    if (!pipe) return;
    Barrier(dst, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    Barrier(quick_background_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageBlit b{};
    b.srcSubresource = b.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    b.srcOffsets[1] = {int32_t(dst.w), int32_t(dst.h), 1};
    b.dstOffsets[1] = {640, 360, 1};
    d_.vkCmdBlitImage(cmd_, dst.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      quick_background_.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &b, VK_FILTER_LINEAR);
    Barrier(quick_background_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    Barrier(dst, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo ca{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    ca.imageView = dst.view; ca.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ca.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
    ri.renderArea = {{0, 0}, {dst.w, dst.h}}; ri.layerCount = 1;
    ri.colorAttachmentCount = 1; ri.pColorAttachments = &ca;
    d_.vkCmdBeginRendering(cmd_, &ri);
    VkViewport vp{0, 0, float(dst.w), float(dst.h), 0, 1};
    VkRect2D sc{{0, 0}, {dst.w, dst.h}};
    d_.vkCmdSetViewport(cmd_, 0, 1, &vp); d_.vkCmdSetScissor(cmd_, 0, 1, &sc);
    d_.vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
    VkDescriptorImageInfo im{VK_NULL_HANDLE, quick_background_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorImageInfo sm{quick_sampler_, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    VkWriteDescriptorSet w[2]{};
    for (auto& x : w) { x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; x.descriptorCount = 1; }
    w[0].dstBinding = 2; w[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; w[0].pImageInfo = &im;
    w[1].dstBinding = 6; w[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; w[1].pImageInfo = &sm;
    d_.vkCmdPushDescriptorSetKHR(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe_layout_, 0, 2, w);
    d_.vkCmdDraw(cmd_, 3, 1, 0, 0);
    d_.vkCmdEndRendering(cmd_);
}

void VulkanBackend::QuickMenu(Image& dst) {
    int percent = ngb_pc_setting(20);
    if (!percent) return;
    uint32_t pixels[NGB_QUICK_WIDTH * NGB_QUICK_HEIGHT];
    if (!ngb_quick_layer(pixels)) return;
    if (!quick_layer_.image &&
        !CreateImage(quick_layer_, kColorFormat, NGB_QUICK_WIDTH, NGB_QUICK_HEIGHT, 1, 1, false, false,
                     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)) return;
    VkDeviceSize at = Upload(pixels, sizeof(pixels), 16);
    Barrier(quick_layer_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkBufferImageCopy c{};
    c.bufferOffset = at;
    c.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    c.imageExtent = {NGB_QUICK_WIDTH, NGB_QUICK_HEIGHT, 1};
    d_.vkCmdCopyBufferToImage(cmd_, ring_, quick_layer_.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &c);
    const uint64_t vs = 0xF113000000000001ull, ps = 0xF113000000000002ull;
    Module(vs, [] {
        return std::string("struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
                           "VO main(uint id : SV_VertexID) { VO o; float2 p = float2((id << 1) & 2, id & 2);\n"
                           "o.uv = p; o.pos = float4(p * 2 - 1, 0, 1); return o; }\n");
    }, true);
    Module(ps, [] {
        return std::string("Texture2D tex0 : register(t0);\nSamplerState smp0 : register(s0);\n"
                           "struct VO { float4 pos : SV_Position; float2 uv : TEXCOORD0; };\n"
                           "float4 main(VO i) : SV_Target { return tex0.Sample(smp0, i.uv); }\n");
    }, false);
    PipeKey k{};
    k.vs = vs; k.ps = ps; k.topo = 2; k.wmask = 0xF; k.has_rt = 1; k.samples = 1;
    k.blend = 1; k.src = 1; k.dst = 0x303;
    VkPipeline pipe = Pipeline(k);
    if (!quick_sampler_) {
        VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        ci.magFilter = ci.minFilter = VK_FILTER_LINEAR;
        ci.addressModeU = ci.addressModeV = ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        d_.vkCreateSampler(dev_, &ci, nullptr, &quick_sampler_);
    }
    if (!pipe || !quick_sampler_) return;
    // Apply the background effect before the crisp menu. Waiting for an
    // action and Quick Menu Pause Off both bypass this pass entirely.
    if (ngb_quick_frozen()) QuickMenuBackground(dst);
    Barrier(quick_layer_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    Barrier(dst, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo ca{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    ca.imageView = dst.view; ca.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ca.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
    ri.renderArea = {{0, 0}, {dst.w, dst.h}}; ri.layerCount = 1;
    ri.colorAttachmentCount = 1; ri.pColorAttachments = &ca;
    d_.vkCmdBeginRendering(cmd_, &ri);
    /* The Xbox front buffer has non-square display pixels. Match the final
     * display aspect, so bottles, weapons and arrows retain their proportions. */
    int mode = ngb_pc_setting(7);
    float aspect = mode == 0 ? 4.0f / 3 : mode == 2 ? 16.0f / 10 : mode == 3 ? 64.0f / 27 : 16.0f / 9;
    float sx = float(dst.w) / (720 * aspect), sy = float(dst.h) / 720;
    float scale = percent / 100.0f;
    sx *= scale; sy *= scale;
    VkViewport vp{0, float(dst.h) - NGB_QUICK_HEIGHT * sy,
                  NGB_QUICK_WIDTH * sx, NGB_QUICK_HEIGHT * sy, 0, 1};
    VkRect2D sc{{0, 0}, {dst.w, dst.h}};
    d_.vkCmdSetViewport(cmd_, 0, 1, &vp); d_.vkCmdSetScissor(cmd_, 0, 1, &sc);
    d_.vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
    VkDescriptorImageInfo im{VK_NULL_HANDLE, quick_layer_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorImageInfo sm{quick_sampler_, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    VkWriteDescriptorSet w[2]{};
    for (auto& x : w) { x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; x.descriptorCount = 1; }
    w[0].dstBinding = 2; w[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; w[0].pImageInfo = &im;
    w[1].dstBinding = 6; w[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; w[1].pImageInfo = &sm;
    d_.vkCmdPushDescriptorSetKHR(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe_layout_, 0, 2, w);
    d_.vkCmdDraw(cmd_, 3, 1, 0, 0);
    d_.vkCmdEndRendering(cmd_);
    Barrier(dst, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
}
