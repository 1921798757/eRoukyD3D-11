// ============================================================================
// D3DFormat.h — DXGI 格式工具
// ============================================================================
// 提供 DXGI_FORMAT 格式的字节大小查询功能。
// 用于 TypedBuffer 等需要知道每个元素占多少字节的场景。
//
// DXGI_FORMAT 命名规则：
//   R = Red, G = Green, B = Blue, A = Alpha
//   D = Depth, S = Stencil
//   数字 = 位数（如 R32 = 32位 Red 通道）
//   后缀 = 数据类型：
//     TYPELESS = 无类型（原始数据）
//     FLOAT    = 浮点数
//     UINT     = 无符号整数
//     SINT     = 有符号整数
//     UNORM    = 无符号归一化（0~1）
//     SNORM    = 有符号归一化（-1~1）
//     SRGB     = sRGB 色彩空间
// ============================================================================

#pragma once

#ifndef D3D_FORMAT_H
#define D3D_FORMAT_H

#include <cstdint>
#include <dxgiformat.h>

// ============================================================================
// GetFormatSize — 获取 DXGI 格式的字节大小
// ============================================================================
// constexpr 表示编译期计算，性能零开销。
// 用于 TypedBuffer 计算总大小：GetFormatSize(format) * numElements
// ============================================================================
constexpr uint32_t GetFormatSize(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_UNKNOWN: 
        return 0;

    // 16 字节格式：4 个 32位分量（128位）
    // 例如：float4（R32G32B32A32_FLOAT）= 4 × 4 = 16 字节
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:    // 4 个 float
    case DXGI_FORMAT_R32G32B32A32_UINT:     // 4 个 uint32
    case DXGI_FORMAT_R32G32B32A32_SINT:     // 4 个 int32
        return 16;

    // 12 字节格式：3 个 32位分量（96位）
    // 例如：float3（R32G32B32_FLOAT）= 3 × 4 = 12 字节
    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:       // 3 个 float（vec3）
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 12;

    // 8 字节格式：2 个 32位分量 或 4 个 16位分量（64位）
    // 例如：float2（R32G32_FLOAT）= 2 × 4 = 8 字节
    //       half4（R16G16B16A16_FLOAT）= 4 × 2 = 8 字节
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:    // 4 个 half（16位浮点）
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:          // 2 个 float（vec2）
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:     // 深度+模板（32+8+24）
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:  // 深度缓冲常用格式
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return 8;

    // 4 字节格式：1 个 32位分量 或 2 个 16位 或 4 个 8位（32位）
    // 例如：float（R32_FLOAT）= 4 字节
    //       float2（R16G16_FLOAT）= 2 × 2 = 4 字节
    //       float4（R8G8B8A8_UNORM）= 4 × 1 = 4 字节
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:  // 10-10-10-2 压缩格式
    case DXGI_FORMAT_R10G10B10A2_UNORM:     // RGB 各 10位，Alpha 2位
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:       // 3 个 11位浮点（紧凑 vec3）
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:        // RGBA 各 8位（最常用纹理格式）
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   // sRGB 色彩空间
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:          // 2 个 half
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:             // 32位深度缓冲
    case DXGI_FORMAT_R32_FLOAT:             // 1 个 float
    case DXGI_FORMAT_R32_UINT:              // 1 个 uint32
    case DXGI_FORMAT_R32_SINT:              // 1 个 int32
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:     // 24位深度 + 8位模板（常用）
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        return 4;

    // 2 字节格式：1 个 16位分量 或 2 个 8位（16位）
    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:            // RG 各 8位
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:             // 1 个 half
    case DXGI_FORMAT_D16_UNORM:             // 16位深度缓冲
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
        return 2;

    // 1 字节格式：1 个 8位分量
    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:              // 单通道 8位（灰度图）
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:              // Alpha 通道
        return 1;

    // 特殊格式
    case DXGI_FORMAT_B5G6R5_UNORM:          // 16位 RGB（5-6-5）
    case DXGI_FORMAT_B5G5R5A1_UNORM:        // 16位 RGBA（5-5-5-1）
        return 2;
    case DXGI_FORMAT_B8G8R8A8_UNORM:        // 32位 BGRA（Windows 常用）
    case DXGI_FORMAT_B8G8R8X8_UNORM:        // 32位 BGRX（无 Alpha）
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return 4;

    default:
        return 0;   // 未知格式返回 0
    }
}



#endif