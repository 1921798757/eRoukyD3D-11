#include "Buffer.h"
#include "XUtil.h"

// ============================================================================
// Buffer 构造函数1 — 简单构造
// ============================================================================
// 根据 bufferDesc 创建缓冲区，并根据 BindFlags 自动创建 SRV/UAV。
// SRV/UAV 使用默认描述（nullptr），D3D11 会自动推断格式。
// 适用于不需要精确控制视图格式的场景（如简单的顶点/索引缓冲区）。
// ============================================================================
Buffer::Buffer(ID3D11Device* d3dDevice, const CD3D11_BUFFER_DESC& bufferDesc)
    : m_ByteWidth(bufferDesc.ByteWidth)
{
    // 第一步：创建 D3D11 缓冲区对象
    // 第二个参数 nullptr 表示不提供初始数据（缓冲区内容为未初始化）
    // GetAddressOf() 返回 ID3D11Buffer** 用于接收创建结果
    d3dDevice->CreateBuffer(&bufferDesc, nullptr, m_pBuffer.GetAddressOf());

    // 第二步：如果绑定标志包含 UAV（无序访问视图），则创建 UAV
    // UAV 允许着色器（主要是 Compute Shader）对缓冲区进行读写操作
    if (bufferDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) {
        d3dDevice->CreateUnorderedAccessView(m_pBuffer.Get(), nullptr, m_pUnorderedAccess.GetAddressOf());
    }

    // 第三步：如果绑定标志包含 SRV（着色器资源视图），则创建 SRV
    // SRV 允许着色器以只读方式访问缓冲区数据
    if (bufferDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
        d3dDevice->CreateShaderResourceView(m_pBuffer.Get(), nullptr, m_pShaderResource.GetAddressOf());
    }
}

// ============================================================================
// Buffer 构造函数2 — 完整构造（自定义 SRV/UAV 描述）
// ============================================================================
// 与构造函数1类似，但允许传入自定义的 SRV 和 UAV 描述。
// 适用于需要精确控制视图格式的场景（如结构化缓冲区、字节地址缓冲区）。
// 例如：StructuredBuffer 需要指定 DXGI_FORMAT_UNKNOWN + 元素范围
// ============================================================================
Buffer::Buffer(ID3D11Device* d3dDevice, const CD3D11_BUFFER_DESC& bufferDesc, 
    const CD3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc,
    const CD3D11_UNORDERED_ACCESS_VIEW_DESC& uavDesc)
    : m_ByteWidth(bufferDesc.ByteWidth)
{
    // 创建缓冲区
    d3dDevice->CreateBuffer(&bufferDesc, nullptr, m_pBuffer.GetAddressOf());

    // 使用自定义的 UAV 描述创建无序访问视图
    if (bufferDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) {
        d3dDevice->CreateUnorderedAccessView(m_pBuffer.Get(), &uavDesc, m_pUnorderedAccess.GetAddressOf());
        
    }

    // 使用自定义的 SRV 描述创建着色器资源视图
    if (bufferDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
        d3dDevice->CreateShaderResourceView(m_pBuffer.Get(), &srvDesc, m_pShaderResource.GetAddressOf());
    }
}

// ============================================================================
// MapDiscard — 映射缓冲区（丢弃模式）
// ============================================================================
// 将 GPU 缓冲区映射到 CPU 地址空间，返回可写内存指针。
// 使用 D3D11_MAP_WRITE_DISCARD 模式：
//   - GPU 驱动会分配一块新的内存给 CPU 写入
//   - CPU 不需要等待 GPU 完成对旧数据的读取（避免管线停顿）
//   - 旧数据在 GPU 完成当前帧渲染后会被自动回收
// 注意：仅适用于 D3D11_USAGE_DYNAMIC 的动态缓冲区
// ============================================================================
void* Buffer::MapDiscard(ID3D11DeviceContext* d3dDeviceContext)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    // Map 的参数说明：
    //   m_pBuffer.Get()  — 要映射的资源
    //   0                — 子资源索引（缓冲区只有 0）
    //   D3D11_MAP_WRITE_DISCARD — 丢弃旧内容，返回新内存
    //   0                — 映射标志（无额外标志）
    //   &mappedResource  — 输出：映射后的数据指针和行跨度
    d3dDeviceContext->Map(m_pBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    return mappedResource.pData;
}

void Buffer::Unmap(ID3D11DeviceContext* d3dDeviceContext)
{
    d3dDeviceContext->Unmap(m_pBuffer.Get(), 0);
}

void Buffer::SetDebugObjectName(std::string_view name)
{
#if (defined(DEBUG) || defined(_DEBUG))

    ::SetDebugObjectName(m_pBuffer.Get(), name);
    if (m_pShaderResource)
        ::SetDebugObjectName(m_pShaderResource.Get(), std::string(name) + ".SRV");
    if (m_pUnorderedAccess)
        ::SetDebugObjectName(m_pUnorderedAccess.Get(), std::string(name) + ".UAV");

#else
    UNREFERENCED_PARAMETER(name);
#endif
}

ByteAddressBuffer::ByteAddressBuffer(ID3D11Device* d3dDevice, uint32_t numUInt32s, uint32_t bindFlags, bool dynamic, bool indirectArgs)
    : Buffer(d3dDevice, 
        CD3D11_BUFFER_DESC(
            numUInt32s * 4, bindFlags,
            dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
            dynamic ? D3D11_CPU_ACCESS_WRITE : 0,
            D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS | (indirectArgs ? D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS : 0)),
        CD3D11_SHADER_RESOURCE_VIEW_DESC(
            m_pBuffer.Get(), DXGI_FORMAT_R32_TYPELESS, 0, numUInt32s, D3D11_BUFFEREX_SRV_FLAG_RAW),
        CD3D11_UNORDERED_ACCESS_VIEW_DESC(
            m_pBuffer.Get(), DXGI_FORMAT_R32_TYPELESS, 0, numUInt32s, D3D11_BUFFER_UAV_FLAG_RAW)),
    m_NumUInt32s(numUInt32s)
{
}
