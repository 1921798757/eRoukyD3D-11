// ============================================================================
// XUtil.h — 通用工具集
// ============================================================================
// 提供项目中常用的工具函数：
//   1. 调试对象命名 — 在 PIX/RenderDoc 中显示可读名称
//   2. 字符串转换 — UTF8 ↔ WString（Windows API 需要宽字符串）
//   3. 字符串哈希 — 将字符串转为唯一 ID（用于资源管理）
//   4. 数学工具 — 法线变换矩阵、线性插值
// ============================================================================

#pragma once

#ifndef XUTIL_H
#define XUTIL_H

#include <DirectXMath.h>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>

// ============================================================================
// 宏定义
// ============================================================================

// LEN_AND_STR：同时传递字符串长度和内容
// 用于某些需要 "长度, 字符串" 两个参数的 Windows API
// 例如：CreateShader(LEN_AND_STR("vs_5_0"), ...)
//       展开为：sizeof("vs_5_0")-1, "vs_5_0"
#define LEN_AND_STR(STR) ((UINT)(sizeof(STR) - 1)), (STR)

// 是否开启图形调试对象名称
// Debug 模式下自动开启，用于在图形调试工具中显示资源名称
#if (defined(DEBUG) || defined(_DEBUG)) && !defined(GRAPHICS_DEBUGGER_OBJECT_NAME)
#define GRAPHICS_DEBUGGER_OBJECT_NAME 1
#endif

// ============================================================================
// SetDebugObjectName — 设置 D3D11 调试对象名称
// ============================================================================
// 模板函数，适用于任何 D3D11 COM 对象（Buffer、Texture、SRV、UAV 等）。
// 作用：在 PIX、RenderDoc 等调试工具中显示可读的名称。
// 原理：通过 SetPrivateData 设置 WKPDID_D3DDebugObjectName 属性。
// 仅在 Debug 模式下有意义，Release 模式下调用不会报错但无效果。
// ============================================================================
template<class IObject>
inline void SetDebugObjectName(IObject* pObject, std::string_view name)
{
    // WKPDID_D3DDebugObjectName 是 D3D 定义的 GUID，标识"调试名称"属性
    pObject->SetPrivateData(WKPDID_D3DDebugObjectName, (uint32_t)name.size(), name.data());
}

// ============================================================================
// 文本转换函数
// ============================================================================
// Windows API 大多使用宽字符串（wchar_t / wstring），
// 而项目代码和文件通常使用 UTF8（char / string）。
// 这两个函数负责在两者之间转换。
// ============================================================================

// 声明 Windows API 的字符转换函数（避免包含整个 windows.h）
// MultiByteToWideChar：多字节字符串 → 宽字符串
// WideCharToMultiByte：宽字符串 → 多字节字符串
#pragma warning(push)
#pragma warning(disable: 28251)
extern "C" __declspec(dllimport) int __stdcall MultiByteToWideChar(unsigned int cp, unsigned long flags, const char* str, int cbmb, wchar_t* widestr, int cchwide);
extern "C" __declspec(dllimport) int __stdcall WideCharToMultiByte(unsigned int cp, unsigned long flags, const wchar_t* widestr, int cchwide, char* str, int cbmb, const char* defchar, int* used_default);
#pragma warning(pop)

// UTF8 字符串 → 宽字符串（wstring）
// 用途：将 UTF8 文件路径、文本转换为 Windows API 需要的宽字符串格式
// 参数 65001 = CP_UTF8，表示输入是 UTF8 编码
inline std::wstring UTF8ToWString(std::string_view utf8str)
{
    if (utf8str.empty()) return std::wstring();
    int cbMultiByte = static_cast<int>(utf8str.size());
    // 第一步：传入 nullptr 获取需要的宽字符数量
    int req = MultiByteToWideChar(65001, 0, utf8str.data(), cbMultiByte, nullptr, 0);
    // 第二步：分配空间并实际转换
    std::wstring res(req, 0);
    MultiByteToWideChar(65001, 0, utf8str.data(), cbMultiByte, &res[0], req);
    return res;
}

// 宽字符串（wstring）→ UTF8 字符串
// 用途：将 Windows API 返回的宽字符串转换为项目内部使用的 UTF8 格式
inline std::string WStringToUTF8(std::wstring_view wstr)
{
    if (wstr.empty()) return std::string();
    int cbMultiByte = static_cast<int>(wstr.size());
    // 第一步：传入 nullptr 获取需要的字节数
    int req = WideCharToMultiByte(65001, 0, wstr.data(), cbMultiByte, nullptr, 0, nullptr, nullptr);
    // 第二步：分配空间并实际转换
    std::string res(req, 0);
    WideCharToMultiByte(65001, 0, wstr.data(), cbMultiByte, &res[0], req, nullptr, nullptr);
    return res;
}

// ============================================================================
// 字符串转 Hash ID
// ============================================================================
// XID 是字符串的哈希值，用于快速比较和查找资源。
// 例如：用材质名称的哈希值作为资源的唯一标识，避免字符串比较的开销。
//
// 用法：
//   XID id = StringToID("myTexture");   // 得到唯一的 size_t 值
//   if (id == StringToID("myTexture"))  // O(1) 比较
// ============================================================================

using XID = size_t;     // 资源 ID 类型（字符串哈希值）
inline XID StringToID(std::string_view str)
{
    static std::hash<std::string_view> hash;   // 标准库字符串哈希函数
    return hash(str);
}

// ============================================================================
// 数学相关函数
// ============================================================================

namespace XMath
{
    // ------------------------------
    // InverseTranspose — 求矩阵的逆的转置
    // ------------------------------
    // 用途：变换法向量。
    //
    // 为什么需要逆的转置？
    //   顶点位置用世界矩阵 M 变换：pos' = M * pos
    //   法向量不能直接用 M 变换（非均匀缩放会破坏垂直关系）
    //   正确做法：normal' = (M^-1)^T * normal
    //
    // 为什么要去掉平移分量？
    //   法向量是方向，不是位置，平移没有意义
    //   将第4行设为 (0,0,0,1) 去掉平移，避免后续变换出错
    //
    // 参数：M — 世界矩阵（4x4）
    // 返回：(M^-1)^T — 逆的转置矩阵（3x3 部分有效）
    inline DirectX::XMMATRIX XM_CALLCONV InverseTranspose(DirectX::FXMMATRIX M)
    {
        using namespace DirectX;

        // 去掉平移分量：将第4行设为 (0, 0, 0, 1)
        XMMATRIX A = M;
        A.r[3] = g_XMIdentityR3;   // g_XMIdentityR3 = (0, 0, 0, 1)

        // 先求逆，再转置
        return XMMatrixTranspose(XMMatrixInverse(nullptr, A));
    }

    // ------------------------------
    // Lerp — 线性插值
    // ------------------------------
    // 在 a 和 b 之间按 t 的比例插值。
    //   t = 0.0 → 返回 a
    //   t = 1.0 → 返回 b
    //   t = 0.5 → 返回 a 和 b 的中间值
    //
    // 用途：动画过渡、颜色混合、位置平滑移动等
    inline float Lerp(float a, float b, float t)
    {
        return (1.0f - t) * a + t * b;
    }
}


#endif
