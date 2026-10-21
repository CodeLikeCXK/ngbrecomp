// The DXGI_FORMAT names SCDA-XBERecomp's vertex code uses (values as Windows defines them); the
// Vulkan backend maps them to VkFormat.
#pragma once
#if defined(_WIN32)
#if defined(__has_include_next) && __has_include_next(<dxgiformat.h>)
#include_next <dxgiformat.h>
#else
#include <dxgiformat.h>
#endif
#else
enum DXGI_FORMAT {
    DXGI_FORMAT_UNKNOWN = 0,
    DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
    DXGI_FORMAT_R32G32B32_FLOAT = 6,
    DXGI_FORMAT_R16G16B16A16_SNORM = 13,
    DXGI_FORMAT_R16G16B16A16_SINT = 14,
    DXGI_FORMAT_R32G32_FLOAT = 16,
    DXGI_FORMAT_R8G8B8A8_UNORM = 28,
    DXGI_FORMAT_R16G16_SNORM = 37,
    DXGI_FORMAT_R16G16_SINT = 38,
    DXGI_FORMAT_R32_FLOAT = 41,
    DXGI_FORMAT_R32_UINT = 42,
    DXGI_FORMAT_R8G8_UNORM = 49,
    DXGI_FORMAT_R16_SNORM = 58,
    DXGI_FORMAT_R16_SINT = 59,
    DXGI_FORMAT_R8_UNORM = 61,
    DXGI_FORMAT_B8G8R8A8_UNORM = 87,
};
#endif

