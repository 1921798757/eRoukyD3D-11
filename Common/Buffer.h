// ============================================================================
// Buffer.h — DirectX 11 缓冲区封装
// ============================================================================
// 本文件定义了四种缓冲区类型，用于在 GPU 上存储和管理数据：
//
//   Buffer              — 基类，封装 ID3D11Buffer + 可选的 SRV/UAV
//   StructuredBuffer<T> — 结构化缓冲区，按 C++ 结构体 T 的布局存储数据
//   TypedBuffer<format> — 类型化缓冲区，按 DXGI_FORMAT 格式存储数据
//   ByteAddressBuffer   — 字节地址缓冲区，支持原始字节级访问
//
// 在着色器中的对应关系：
//   StructuredBuffer   → StructuredBuffer<T> / RWStructuredBuffer<T>
//   TypedBuffer        → Buffer<T> / RWBuffer<T>
//   ByteAddressBuffer  → ByteAddressBuffer / RWByteAddressBuffer
// ============================================================================

#pragma once

#ifndef BUFFER_H
#define BUFFER_H

#include "WinMin.h"
#include "D3DFormat.h"
#include <d3d11_1.h>
#include <wrl/client.h>       // ComPtr 智能指针，自动管理 COM 对象引用计数
#include <vector>
#include <string>
#include <string_view>

// ============================================================================
// Buffer — 所有缓冲区的基类
// ============================================================================
// 封装了 D3D11 缓冲区的三个核心 COM 对象：
//   1. ID3D11Buffer            — 缓冲区资源本身 
//   2. ID3D11ShaderResourceView (SRV) — 让着色器以只读方式访问缓冲区
//   3. ID3D11UnorderedAccessView (UAV) — 让着色器以读写方式访问缓冲区（Compute Shader 常用）
//
// 使用场景：
//   - 顶点/索引缓冲区通常只需要 Buffer 本身（不需要 SRV/UAV）
//   - Compute Shader 的数据缓冲区通常需要 SRV + UAV
//   - 动态缓冲区通过 MapDiscard/Unmap 从 CPU 端更新数据
// ============================================================================
class Buffer
{
public:
    // 构造函数1：简单构造，SRV/UAV 使用默认描述（由 BindFlags 决定是否创建）
    Buffer(ID3D11Device* d3dDevice, const CD3D11_BUFFER_DESC& bufferDesc);

    // 构造函数2：完整构造，可自定义 SRV 和 UAV 的描述
    // 当需要精确控制视图格式（如指定元素范围、计数器标志等）时使用此版本
    Buffer(ID3D11Device* d3dDevice, const CD3D11_BUFFER_DESC& bufferDesc, 
        const CD3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc, 
        const CD3D11_UNORDERED_ACCESS_VIEW_DESC& uavDesc);
    ~Buffer() = default;

    // 禁止拷贝（COM 对象不应被拷贝），但允许移动语义（转移所有权）
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) = default;
    Buffer& operator=(Buffer&&) = default;

    // 获取底层 D3D11 资源指针，用于绑定到渲染管线
    ID3D11Buffer* GetBuffer() { return m_pBuffer.Get(); }
    ID3D11UnorderedAccessView* GetUnorderedAccess() { return m_pUnorderedAccess.Get(); }   // 获取 UAV（着色器读写视图）
    ID3D11ShaderResourceView* GetShaderResource() { return m_pShaderResource.Get(); }      // 获取 SRV（着色器只读视图）

    // MapDiscard：将缓冲区映射到 CPU 内存，返回数据指针
    // - 使用 D3D11_MAP_WRITE_DISCARD 模式：GPU 会分配一块新内存，CPU 写入新数据
    // - 优点：CPU 不需要等待 GPU 完成对旧数据的读取，避免管线停顿
    // - 仅适用于 D3D11_USAGE_DYNAMIC 的动态缓冲区
    // TODO: 支持 NOOVERWRITE 环形缓冲区？（可以让 CPU 和 GPU 同时写入不同区域）
    void* MapDiscard(ID3D11DeviceContext* d3dDeviceContext);

    // Unmap：完成数据写入后，解除映射，让 GPU 可以使用新数据
    void Unmap(ID3D11DeviceContext* d3dDeviceContext);

    // 获取缓冲区的字节大小
    uint32_t GetByteWidth() const { return m_ByteWidth; }

    // 设置调试对象名（仅在 Debug 模式下生效）
    // 作用：在图形调试工具（如 PIX、RenderDoc）中显示可读的名称，方便定位问题
    void SetDebugObjectName(std::string_view name);

protected:
    // ComPtr 别名：WRL 智能指针，自动管理 COM 对象的 AddRef/Release
    template<class Type>
    using ComPtr = Microsoft::WRL::ComPtr<Type>;

    ComPtr<ID3D11Buffer> m_pBuffer;                          // 缓冲区资源
    ComPtr<ID3D11ShaderResourceView> m_pShaderResource;      // 着色器资源视图（只读）
    ComPtr<ID3D11UnorderedAccessView> m_pUnorderedAccess;    // 无序访问视图（读写）
    uint32_t m_ByteWidth = 0;                                // 缓冲区大小（字节）
};


// ============================================================================
// StructuredBuffer<T> — 结构化缓冲区
// ============================================================================
// 数据按 C++ 结构体 T 的布局在 GPU 上存储，每个元素大小为 sizeof(T)。
// GPU 着色器中需要定义相同布局的结构体来访问数据。
//
// 特点：
//   - 元素类型由 C++ 模板参数 T 决定（如 Vertex、Particle 等自定义结构体）
//   - 支持可选的追加/消费计数器（append/consume counter）
//   - 可同时作为 SRV（只读）和 UAV（读写）使用
//
// 着色器对应：
//   StructuredBuffer<MyStruct>     — 只读访问
//   RWStructuredBuffer<MyStruct>   — 读写访问
//
// 注意：确保 T 与着色器中结构体的大小/布局相同（注意对齐和填充）
// ============================================================================
template<class T>
class StructuredBuffer : public Buffer
{
public:
    // 参数说明：
    //   elements    — 元素个数（不是字节数！）
    //   bindFlags   — 绑定标志，默认同时支持 UAV 和 SRV
    //   enableCounter — 是否启用追加/消费计数器（用于 AppendBuffer/ConsumeBuffer）
    //   dynamic     — true 表示动态缓冲区（CPU 可频繁更新），false 表示静态缓冲区
    StructuredBuffer(ID3D11Device* d3dDevice, uint32_t elements,
        uint32_t bindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,
        bool enableCounter = false,
        bool dynamic = false);
    ~StructuredBuffer() = default;

    // 禁止拷贝，允许移动
    StructuredBuffer(const StructuredBuffer&) = delete;
    StructuredBuffer& operator=(const StructuredBuffer&) = delete;
    StructuredBuffer(StructuredBuffer&&) = default;
    StructuredBuffer& operator=(StructuredBuffer&&) = default;

    // 映射缓冲区到 CPU 内存，返回类型化的 T* 指针
    // 仅适用于动态缓冲区（dynamic=true）
    // TODO: 支持 NOOVERWRITE 环形缓冲区？
    T* MapDiscard(ID3D11DeviceContext* d3dDeviceContext);

    // 获取元素个数
    uint32_t GetNumElements() const { return m_Elements; }


private:
    uint32_t m_Elements;   // 元素个数
};

// StructuredBuffer 构造函数实现
// 关键参数：
//   D3D11_RESOURCE_MISC_BUFFER_STRUCTURED — 告诉 D3D11 这是一个结构化缓冲区
//   sizeof(T) — 每个元素的结构体大小（StructureByteStride）
//   SRV 使用 DXGI_FORMAT_UNKNOWN — 结构化缓冲区不需要指定格式，由结构体布局决定
//   UAV 的 D3D11_BUFFER_UAV_FLAG_COUNTER — 启用追加/消费计数器
template<class T>
inline StructuredBuffer<T>::StructuredBuffer(ID3D11Device* d3dDevice, uint32_t elements, uint32_t bindFlags, bool enableCounter, bool dynamic)
    : m_Elements(elements), //初始化成员变量
    Buffer(d3dDevice,       //调用Buffer构造函数
        // 缓冲区描述：大小 = 元素大小 × 元素个数
        CD3D11_BUFFER_DESC(sizeof(T) * elements, bindFlags,
        dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,    // 动态/静态使用方式
        dynamic ? D3D11_CPU_ACCESS_WRITE : 0,                   // 动态缓冲区允许 CPU 写入
        D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,                   // 标记为结构化缓冲区
        sizeof(T)),                                              // 每个元素的结构体字节跨度
        // SRV 描述：缓冲区维度，格式未知（由结构体决定），从第0个元素开始，共 elements 个
        CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, 0, elements),
        // UAV 描述：缓冲区维度，格式未知，从第0个元素开始，共 elements 个
        // 如果启用计数器，使用 D3D11_BUFFER_UAV_FLAG_COUNTER 标志
        CD3D11_UNORDERED_ACCESS_VIEW_DESC(D3D11_UAV_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, 0, elements, 0,
            enableCounter ? D3D11_BUFFER_UAV_FLAG_COUNTER : 0))
{
}

// 将结构化缓冲区映射到 CPU，返回类型安全的 T* 指针
template <typename T>
T* StructuredBuffer<T>::MapDiscard(ID3D11DeviceContext* d3dDeviceContext)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    // D3D11_MAP_WRITE_DISCARD：丢弃旧内容，返回一块新的可写内存
    // GPU 不会等待，CPU 也不会等待 GPU，性能最优
    d3dDeviceContext->Map(m_pBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    return static_cast<T*>(mappedResource.pData);
}

// ============================================================================
// TypedBuffer<format> — 类型化缓冲区
// ============================================================================
// 数据按 DXGI_FORMAT 格式存储，每个元素是固定格式的标量/向量。
// 例如 DXGI_FORMAT_R32G32B32_FLOAT 表示每个元素是 3 个 float（vec3）。
//
// 与 StructuredBuffer 的区别：
//   - TypedBuffer 使用 DXGI 格式描述每个元素（适合简单类型）
//   - StructuredBuffer 使用 C++ 结构体描述每个元素（适合复杂类型）
//
// 着色器对应：
//   Buffer<float3>    — 只读访问
//   RWBuffer<float3>  — 读写访问
// ============================================================================
template <DXGI_FORMAT format>
struct TypedBuffer : public Buffer
{
public:
    // 参数说明：
    //   numElems  — 元素个数
    //   bindFlags — 绑定标志，默认同时支持 UAV 和 SRV
    //   dynamic   — true 表示动态缓冲区
    TypedBuffer(ID3D11Device* d3dDevice, uint32_t numElems,
        uint32_t bindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,
        bool dynamic = false);
    ~TypedBuffer() = default;

    // 禁止拷贝，允许移动
    TypedBuffer(const TypedBuffer&) = delete;
    TypedBuffer& operator=(const TypedBuffer&) = delete;
    TypedBuffer(TypedBuffer&&) = default;
    TypedBuffer& operator=(TypedBuffer&&) = default;

    // 获取元素个数
    uint32_t GetNumElements() const { return m_Elements; }

private:
    uint32_t m_Elements;   // 元素个数
};

// TypedBuffer 构造函数实现
// 关键：使用 GetFormatSize(format) 获取每个 DXGI 格式的字节大小
// SRV 和 UAV 都使用指定的 format 格式
template<DXGI_FORMAT format>
TypedBuffer<format>::TypedBuffer(ID3D11Device* d3dDevice, uint32_t numElems, uint32_t bindFlags, bool dynamic)
    : m_Elements(numElems), Buffer(d3dDevice, CD3D11_BUFFER_DESC(
        GetFormatSize(format) * numElems, bindFlags,                          // 总大小 = 元素字节大小 × 元素 个数
        dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
        dynamic ? D3D11_CPU_ACCESS_WRITE : 0),
        CD3D11_SHADER_RESOURCE_VIEW_DESC(m_pBuffer.Get(), format, 0, numElems),  // SRV 使用指定格式
        CD3D11_UNORDERED_ACCESS_VIEW_DESC(m_pBuffer.Get(), format, 0, numElems)) // UAV 使用指定格式
{
}

// ============================================================================
// ByteAddressBuffer — 字节地址缓冲区
// ============================================================================
// 最灵活的缓冲区类型，支持按任意 4 字节对齐的字节偏移访问数据。
// 数据在着色器中以 uint（32位）为基本单位进行读写。
//
// 特点：
//   - 使用 D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS 标志启用原始视图
//   - SRV/UAV 使用 DXGI_FORMAT_R32_TYPELESS + RAW 标志
//   - 可选支持 D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS（用于间接绘制参数）
//
// 着色器对应：
//   ByteAddressBuffer       — 只读访问（用 Load/Load2/Load3/Load4）
//   RWByteAddressBuffer     — 读写访问（用 Load/Store）
// ============================================================================
struct ByteAddressBuffer : public Buffer
{
public:
    // 参数说明：
    //   numUInt32s   — 缓冲区大小（以 uint32 为单位，总字节数 = numUInt32s × 4）
    //   bindFlags    — 绑定标志
    //   dynamic      — 是否为动态缓冲区
    //   indirectArgs — 是否用于 DrawIndirect 的参数缓冲区 
    ByteAddressBuffer(ID3D11Device* d3dDevice, uint32_t numUInt32s,
        uint32_t bindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,
        bool dynamic = false, bool indirectArgs = false);
    ~ByteAddressBuffer() = default;

    // 禁止拷贝，允许移动
    ByteAddressBuffer(const ByteAddressBuffer&) = delete;
    ByteAddressBuffer& operator=(const ByteAddressBuffer&) = delete;
    ByteAddressBuffer(ByteAddressBuffer&&) = default;
    ByteAddressBuffer& operator=(ByteAddressBuffer&&) = default;

    // 获取缓冲区包含的 uint32 个数
    uint32_t GetNumUInt32s() const { return m_NumUInt32s; }

private:
    uint32_t m_NumUInt32s;   // uint32 个数
};

#endif


