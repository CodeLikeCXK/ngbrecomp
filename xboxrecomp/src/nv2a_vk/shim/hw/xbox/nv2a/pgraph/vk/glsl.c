/*
 * Geforce NV2A PGRAPH Vulkan Renderer
 *
 * Copyright (c) 2024-2025 Matt Borgerson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "ui/xemu-settings.h"
#include "renderer.h"

#include <assert.h>
#include <glslang/Include/glslang_c_interface.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "qemu/fast-hash.h"

static const glslang_resource_t
    resource_limits = { .max_lights = 32,
                        .max_clip_planes = 6,
                        .max_texture_units = 32,
                        .max_texture_coords = 32,
                        .max_vertex_attribs = 64,
                        .max_vertex_uniform_components = 4096,
                        .max_varying_floats = 64,
                        .max_vertex_texture_image_units = 32,
                        .max_combined_texture_image_units = 80,
                        .max_texture_image_units = 32,
                        .max_fragment_uniform_components = 4096,
                        .max_draw_buffers = 32,
                        .max_vertex_uniform_vectors = 128,
                        .max_varying_vectors = 8,
                        .max_fragment_uniform_vectors = 16,
                        .max_vertex_output_vectors = 16,
                        .max_fragment_input_vectors = 15,
                        .min_program_texel_offset = -8,
                        .max_program_texel_offset = 7,
                        .max_clip_distances = 8,
                        .max_compute_work_group_count_x = 65535,
                        .max_compute_work_group_count_y = 65535,
                        .max_compute_work_group_count_z = 65535,
                        .max_compute_work_group_size_x = 1024,
                        .max_compute_work_group_size_y = 1024,
                        .max_compute_work_group_size_z = 64,
                        .max_compute_uniform_components = 1024,
                        .max_compute_texture_image_units = 16,
                        .max_compute_image_uniforms = 8,
                        .max_compute_atomic_counters = 8,
                        .max_compute_atomic_counter_buffers = 1,
                        .max_varying_components = 60,
                        .max_vertex_output_components = 64,
                        .max_geometry_input_components = 64,
                        .max_geometry_output_components = 128,
                        .max_fragment_input_components = 128,
                        .max_image_units = 8,
                        .max_combined_image_units_and_fragment_outputs = 8,
                        .max_combined_shader_output_resources = 8,
                        .max_image_samples = 0,
                        .max_vertex_image_uniforms = 0,
                        .max_tess_control_image_uniforms = 0,
                        .max_tess_evaluation_image_uniforms = 0,
                        .max_geometry_image_uniforms = 0,
                        .max_fragment_image_uniforms = 8,
                        .max_combined_image_uniforms = 8,
                        .max_geometry_texture_image_units = 16,
                        .max_geometry_output_vertices = 256,
                        .max_geometry_total_output_components = 1024,
                        .max_geometry_uniform_components = 1024,
                        .max_geometry_varying_components = 64,
                        .max_tess_control_input_components = 128,
                        .max_tess_control_output_components = 128,
                        .max_tess_control_texture_image_units = 16,
                        .max_tess_control_uniform_components = 1024,
                        .max_tess_control_total_output_components = 4096,
                        .max_tess_evaluation_input_components = 128,
                        .max_tess_evaluation_output_components = 128,
                        .max_tess_evaluation_texture_image_units = 16,
                        .max_tess_evaluation_uniform_components = 1024,
                        .max_tess_patch_components = 120,
                        .max_patch_vertices = 32,
                        .max_tess_gen_level = 64,
                        .max_viewports = 16,
                        .max_vertex_atomic_counters = 0,
                        .max_tess_control_atomic_counters = 0,
                        .max_tess_evaluation_atomic_counters = 0,
                        .max_geometry_atomic_counters = 0,
                        .max_fragment_atomic_counters = 8,
                        .max_combined_atomic_counters = 8,
                        .max_atomic_counter_bindings = 1,
                        .max_vertex_atomic_counter_buffers = 0,
                        .max_tess_control_atomic_counter_buffers = 0,
                        .max_tess_evaluation_atomic_counter_buffers = 0,
                        .max_geometry_atomic_counter_buffers = 0,
                        .max_fragment_atomic_counter_buffers = 1,
                        .max_combined_atomic_counter_buffers = 1,
                        .max_atomic_counter_buffer_size = 16384,
                        .max_transform_feedback_buffers = 4,
                        .max_transform_feedback_interleaved_components = 64,
                        .max_cull_distances = 8,
                        .max_combined_clip_and_cull_distances = 8,
                        .max_samples = 4,
                        .max_mesh_output_vertices_nv = 256,
                        .max_mesh_output_primitives_nv = 512,
                        .max_mesh_work_group_size_x_nv = 32,
                        .max_mesh_work_group_size_y_nv = 1,
                        .max_mesh_work_group_size_z_nv = 1,
                        .max_task_work_group_size_x_nv = 32,
                        .max_task_work_group_size_y_nv = 1,
                        .max_task_work_group_size_z_nv = 1,
                        .max_mesh_view_count_nv = 4,
                        .maxDualSourceDrawBuffersEXT = 1,
                        .limits = {
                            .non_inductive_for_loops = 1,
                            .while_loops = 1,
                            .do_while_loops = 1,
                            .general_uniform_indexing = 1,
                            .general_attribute_matrix_vector_indexing = 1,
                            .general_varying_indexing = 1,
                            .general_sampler_indexing = 1,
                            .general_variable_indexing = 1,
                            .general_constant_matrix_vector_indexing = 1,
                        } };

void pgraph_vk_init_glsl_compiler(void)
{
    glslang_initialize_process();
}

void pgraph_vk_finalize_glsl_compiler(void)
{
    glslang_finalize_process();
}

/* xboxrecomp: SPIR-V kept on disk, by the GLSL it came from.
 *
 * glslang runs on the PFIFO thread, at the draw that first needs a shader,
 * and costs a few milliseconds a shader: every new effect on screen -- a
 * fight starting, the first sword swing -- dropped a run of frames, 100+
 * ms at a time, again every session. The driver caches its own half
 * (Mesa's shader cache); this caches ours. NV2A_SHADER_CACHE names the
 * directory; unset, nothing is cached. Two independent 64-bit hashes of
 * the stage and source name each entry, and the source length is checked
 * on load. */
#define SPV_CACHE_VERSION 1

static uint64_t fnv1a64(const void *p, size_t n, uint64_t h)
{
    const uint8_t *b = p;
    while (n--)
        h = (h ^ *b++) * 0x100000001b3ull;
    return h;
}

static bool spv_cache_path(char *out, size_t n, glslang_stage_t stage,
                           const char *src, size_t len)
{
    static const char *dir;
    static int checked;
    if (!checked) {
        dir = getenv("NV2A_SHADER_CACHE");
        if (dir && *dir)
            g_mkdir_with_parents(dir, 0755);
        else
            dir = NULL;
        checked = 1;
    }
    if (!dir)
        return false;
    uint32_t salt[2] = { SPV_CACHE_VERSION, (uint32_t)stage };
    uint64_t h1 = fnv1a64(src, len, fnv1a64(salt, sizeof salt, 0xcbf29ce484222325ull));
    uint64_t h2 = fast_hash((const uint8_t *)src, len) ^ ((uint64_t)stage << 56);
    snprintf(out, n, "%s/%016llx%016llx.spv", dir,
             (unsigned long long)h1, (unsigned long long)h2);
    return true;
}

static GByteArray *spv_cache_load(const char *path, size_t len)
{
    FILE *f = fopen(path, "rb");
    uint64_t stored_len;
    long size;
    GByteArray *a = NULL;

    if (!f)
        return NULL;
    if (fread(&stored_len, sizeof stored_len, 1, f) == 1 && stored_len == len &&
        fseek(f, 0, SEEK_END) == 0 && (size = ftell(f)) > (long)sizeof stored_len &&
        (size - (long)sizeof stored_len) % 4 == 0 &&
        fseek(f, sizeof stored_len, SEEK_SET) == 0) {
        size -= sizeof stored_len;
        a = g_byte_array_sized_new(size);
        g_byte_array_set_size(a, size);
        if (fread(a->data, 1, size, f) != (size_t)size) {
            g_byte_array_free(a, TRUE);
            a = NULL;
        }
    }
    fclose(f);
    return a;
}

static void spv_cache_store(const char *path, size_t len, const GByteArray *spv)
{
    char tmp[4200];
    uint64_t stored_len = len;
    FILE *f;

    snprintf(tmp, sizeof tmp, "%s.%d.tmp", path, (int)getpid());
    f = fopen(tmp, "wb");
    if (!f)
        return;
    bool ok = fwrite(&stored_len, sizeof stored_len, 1, f) == 1 &&
              fwrite(spv->data, 1, spv->len, f) == spv->len;
    ok = (fclose(f) == 0) && ok;
    if (!ok || rename(tmp, path) != 0)
        remove(tmp);
}

static GByteArray *compile_glsl_to_spv_real(glslang_stage_t stage, const char *glsl_source);

static GByteArray *compile_cached(glslang_stage_t stage, const char *glsl_source)
{
    char path[4096];
    size_t len = strlen(glsl_source);
    bool cached = spv_cache_path(path, sizeof path, stage, glsl_source, len);
    GByteArray *spv;

    if (cached && (spv = spv_cache_load(path, len)))
        return spv;
    spv = compile_glsl_to_spv_real(stage, glsl_source);
    if (cached && spv)
        spv_cache_store(path, len, spv);
    return spv;
}

/* For the startup pipeline prewarm (draw.c): from the disk cache when
 * there, compiled into it when not. Any thread. */
GByteArray *pgraph_vk_compile_glsl_to_spv_cached(glslang_stage_t stage,
                                                 const char *glsl_source)
{
    return compile_cached(stage, glsl_source);
}

/* Results of pgraph_vk_precompile_glsl waiting to be asked for. Only the
 * PFIFO thread touches this; the worker threads only compile. */
#define STASH_SIZE 4
static struct {
    glslang_stage_t stage;
    char *src;
    GByteArray *spv;
} s_stash[STASH_SIZE];

GByteArray *pgraph_vk_compile_glsl_to_spv(glslang_stage_t stage,
                                          const char *glsl_source)
{
    for (int i = 0; i < STASH_SIZE; i++) {
        if (s_stash[i].src && s_stash[i].stage == stage &&
            !strcmp(s_stash[i].src, glsl_source)) {
            GByteArray *spv = s_stash[i].spv;
            g_free(s_stash[i].src);
            s_stash[i].src = NULL;
            s_stash[i].spv = NULL;
            return spv;
        }
    }
    return compile_cached(stage, glsl_source);
}

/* A new shader needs its vertex, fragment and sometimes geometry stages,
 * and glslang took them one after another on the PFIFO thread: a few ms
 * each, all of it a stalled frame. They are independent, so they compile
 * side by side here and the stall is the slowest stage, not the sum. */
typedef struct {
    glslang_stage_t stage;
    const char *src;
    GByteArray *spv;
} PrecompileJob;

static void *precompile_worker(void *arg)
{
    PrecompileJob *job = arg;
    job->spv = compile_cached(job->stage, job->src);
    return NULL;
}

void pgraph_vk_precompile_glsl(const glslang_stage_t *stages,
                               const char *const *sources, int n)
{
    PrecompileJob jobs[STASH_SIZE];
    QemuThread threads[STASH_SIZE];

    for (int i = 0; i < STASH_SIZE; i++) {   /* never asked for */
        if (s_stash[i].src) {
            g_free(s_stash[i].src);
            g_byte_array_unref(s_stash[i].spv);
            s_stash[i].src = NULL;
            s_stash[i].spv = NULL;
        }
    }
    if (n < 2)
        return;     /* nothing to overlap */
    if (n > STASH_SIZE)
        n = STASH_SIZE;
    for (int i = 0; i < n; i++)
        jobs[i] = (PrecompileJob){ stages[i], sources[i], NULL };
    for (int i = 1; i < n; i++)
        qemu_thread_create(&threads[i], "glslang", precompile_worker,
                           &jobs[i], QEMU_THREAD_JOINABLE);
    precompile_worker(&jobs[0]);
    for (int i = 1; i < n; i++)
        qemu_thread_join(&threads[i]);

    for (int i = 0; i < n; i++) {
        int slot = 0;
        while (slot < STASH_SIZE && s_stash[slot].src)
            slot++;
        if (slot == STASH_SIZE || !jobs[i].spv) {
            if (jobs[i].spv)
                g_byte_array_unref(jobs[i].spv);
            continue;
        }
        s_stash[slot].stage = jobs[i].stage;
        s_stash[slot].src = g_strdup(jobs[i].src);
        s_stash[slot].spv = jobs[i].spv;
    }
}

static GByteArray *compile_glsl_to_spv_real(glslang_stage_t stage, const char *glsl_source)
{
    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_3,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_6,
        .code = glsl_source,
        .default_version = 460,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = &resource_limits,
    };

    glslang_shader_t *shader = glslang_shader_create(&input);

    if (!glslang_shader_preprocess(shader, &input)) {
        fprintf(stderr,
                "GLSL preprocessing failed\n"
                "[INFO]: %s\n"
                "[DEBUG]: %s\n"
                "%s\n",
                glslang_shader_get_info_log(shader),
                glslang_shader_get_info_debug_log(shader), input.code);
        assert(!"glslang preprocess failed");
        glslang_shader_delete(shader);
        return NULL;
    }

    if (!glslang_shader_parse(shader, &input)) {
        fprintf(stderr,
                "GLSL parsing failed\n"
                "[INFO]: %s\n"
                "[DEBUG]: %s\n"
                "%s\n",
                glslang_shader_get_info_log(shader),
                glslang_shader_get_info_debug_log(shader),
                glslang_shader_get_preprocessed_code(shader));
        assert(!"glslang parse failed");
        glslang_shader_delete(shader);
        return NULL;
    }

    glslang_program_t *program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT |
                                           GLSLANG_MSG_VULKAN_RULES_BIT)) {
        fprintf(stderr,
                "GLSL linking failed\n"
                "[INFO]: %s\n"
                "[DEBUG]: %s\n",
                glslang_program_get_info_log(program),
                glslang_program_get_info_debug_log(program));
        assert(!"glslang link failed");
        glslang_program_delete(program);
        glslang_shader_delete(shader);
        return NULL;
    }

    glslang_spv_options_t spv_options = {
        .validate = true,
    };

    if (g_config.display.vulkan.debug_shaders) {
        spv_options.disable_optimizer = true;
        spv_options.generate_debug_info = true;
        spv_options.emit_nonsemantic_shader_debug_info = true;
        spv_options.emit_nonsemantic_shader_debug_source = true;

        // XXX: Note emit_nonsemantic_shader_debug_source actually does nothing
        // as of 2024.07.25. To actually get glsl source embedded in spv, we
        // must do the following...
        //
        // ref: https://github.com/KhronosGroup/glslang/issues/3252
        glslang_program_add_source_text(program, input.stage, input.code,
                                        strlen(input.code));
    }
    glslang_program_SPIRV_generate_with_options(program, stage, &spv_options);

    const char *spirv_messages = glslang_program_SPIRV_get_messages(program);
    if (spirv_messages) {
        printf("%s\b", spirv_messages);
    }

    size_t num_program_bytes =
        glslang_program_SPIRV_get_size(program) * sizeof(uint32_t);

    guint8 *data = g_malloc(num_program_bytes);
    glslang_program_SPIRV_get(program, (unsigned int *)data);

    glslang_program_delete(program);
    glslang_shader_delete(shader);

    return g_byte_array_new_take(data, num_program_bytes);
}

VkShaderModule pgraph_vk_create_shader_module_from_spv(PGRAPHVkState *r, GByteArray *spv)
{
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spv->len,
        .pCode = (uint32_t *)spv->data,
    };
    VkShaderModule module;
    VK_CHECK(
        vkCreateShaderModule(r->device, &create_info, NULL, &module));
    return module;
}

static void block_to_uniforms(const SpvReflectBlockVariable *block, ShaderUniformLayout *layout)
{
    assert(!layout->uniforms);

    layout->num_uniforms = block->member_count;
    layout->uniforms = g_malloc0_n(block->member_count, sizeof(ShaderUniform));
    layout->total_size = block->size;
    layout->allocation = g_malloc0(block->size);

    for (uint32_t k = 0; k < block->member_count; ++k) {
        const SpvReflectBlockVariable *member = &block->members[k];

        assert(member->array.dims_count < 2);

        int dim = 1;
        for (int i = 0; i < member->array.dims_count; i++) {
            dim *= member->array.dims[i];
        }
        int stride = MAX(member->array.stride, member->numeric.matrix.stride);
        if (member->numeric.matrix.column_count) {
            dim *= member->numeric.matrix.column_count;
            if (member->array.stride) {
                stride =
                    member->array.stride / member->numeric.matrix.column_count;
            }
        }
        layout->uniforms[k] = (ShaderUniform){
            .name = strdup(member->name),
            .offset = member->offset,
            .dim_v = MAX(1, member->numeric.vector.component_count),
            .dim_a = dim,
            .stride = stride,
        };

        // fprintf(stderr, "<%s offset=%zd dim_v=%zd dim_a=%zd stride=%zd>\n",
        //     layout->uniforms[k].name,
        //     layout->uniforms[k].offset,
        //     layout->uniforms[k].dim_v,
        //     layout->uniforms[k].dim_a,
        //     layout->uniforms[k].stride
        //     );
    }
    // fprintf(stderr, "--\n");
}

static void init_layout_from_spv(ShaderModuleInfo *info)
{
    SpvReflectResult result = spvReflectCreateShaderModule(
        info->spirv->len, info->spirv->data, &info->reflect_module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS &&
           "Failed to create SPIR-V shader module");

    uint32_t descriptor_set_count = 0;
    result = spvReflectEnumerateDescriptorSets(&info->reflect_module,
                                               &descriptor_set_count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS &&
           "Failed to enumerate descriptor sets");

    info->descriptor_sets =
        g_malloc_n(descriptor_set_count, sizeof(SpvReflectDescriptorSet *));
    result = spvReflectEnumerateDescriptorSets(
        &info->reflect_module, &descriptor_set_count, info->descriptor_sets);
    assert(result == SPV_REFLECT_RESULT_SUCCESS &&
           "Failed to enumerate descriptor sets");

    info->uniforms.num_uniforms = 0;
    info->uniforms.uniforms = NULL;

    for (uint32_t i = 0; i < descriptor_set_count; ++i) {
        const SpvReflectDescriptorSet *descriptor_set =
            info->descriptor_sets[i];
        for (uint32_t j = 0; j < descriptor_set->binding_count; ++j) {
            const SpvReflectDescriptorBinding *binding =
                descriptor_set->bindings[j];
            if (binding->descriptor_type !=
                SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                continue;
            }

            const SpvReflectBlockVariable *block = &binding->block;
            block_to_uniforms(block, &info->uniforms);
        }
    }

    info->push_constants.num_uniforms = 0;
    info->push_constants.uniforms = NULL;
    assert(info->reflect_module.push_constant_block_count < 2);
    if (info->reflect_module.push_constant_block_count) {
        block_to_uniforms(&info->reflect_module.push_constant_blocks[0],
                          &info->push_constants);
    }
}

static glslang_stage_t vk_shader_stage_to_glslang_stage(VkShaderStageFlagBits stage)
{
    switch (stage) {
    case VK_SHADER_STAGE_GEOMETRY_BIT:
        return GLSLANG_STAGE_GEOMETRY;
    case VK_SHADER_STAGE_VERTEX_BIT:
        return GLSLANG_STAGE_VERTEX;
    case VK_SHADER_STAGE_FRAGMENT_BIT:
        return GLSLANG_STAGE_FRAGMENT;
    case VK_SHADER_STAGE_COMPUTE_BIT:
        return GLSLANG_STAGE_COMPUTE;
    default:
        assert(0);
    }
}

ShaderModuleInfo *pgraph_vk_create_shader_module_from_glsl(
    PGRAPHVkState *r, VkShaderStageFlagBits stage, const char *glsl)
{
    ShaderModuleInfo *info = g_malloc0(sizeof(*info));
    info->refcnt = 0;
    info->glsl = strdup(glsl);
    info->spirv = pgraph_vk_compile_glsl_to_spv(
        vk_shader_stage_to_glslang_stage(stage), glsl);
    info->module = pgraph_vk_create_shader_module_from_spv(r, info->spirv);
    init_layout_from_spv(info);
    return info;
}

static void finalize_uniform_layout(ShaderUniformLayout *layout)
{
    for (int i = 0; i < layout->num_uniforms; i++) {
        free((void*)layout->uniforms[i].name);
    }
    if (layout->uniforms) {
        g_free(layout->uniforms);
    }
}

void pgraph_vk_ref_shader_module(ShaderModuleInfo *info)
{
    info->refcnt++;
}

void pgraph_vk_unref_shader_module(PGRAPHVkState *r, ShaderModuleInfo *info)
{
    assert(info->refcnt >= 1);

    info->refcnt--;
    if (info->refcnt == 0) {
        pgraph_vk_destroy_shader_module(r, info);
    }
}

void pgraph_vk_destroy_shader_module(PGRAPHVkState *r, ShaderModuleInfo *info)
{
    assert(info->refcnt == 0);
    if (info->glsl) {
        free(info->glsl);
    }
    finalize_uniform_layout(&info->uniforms);
    finalize_uniform_layout(&info->push_constants);
    free(info->descriptor_sets);
    spvReflectDestroyShaderModule(&info->reflect_module);
    vkDestroyShaderModule(r->device, info->module, NULL);
    g_byte_array_unref(info->spirv);
    g_free(info);
}
