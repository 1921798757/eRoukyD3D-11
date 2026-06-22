#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
#include <wincodec.h>   // WIC for decoding bitmaps
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D3DApp(hInstance, windowName, initWidth, initHeight),
    m_IndexCount(),             // 初始化为0，后面InitResource会设置具体值	                
    m_CurrFrame(),
    m_CurrMode(ShowMode::WoodCrate),
    m_VSConstantBuffer(),       // 初始化为零
    m_PSConstantBuffer()        // 初始化为零
{
}

GameApp::~GameApp()
{
}

//---------------------------------------------------------
// Init - 初始化游戏
// 调用链：D3DApp::Init() → InitEffect() → InitResource()
//---------------------------------------------------------
bool GameApp::Init()
{
    if (!D3DApp::Init())
        return false;

    if (!InitEffect())
        return false;

    if (!InitResource())
        return false;

    return true;
}

//---------------------------------------------------------
// OnResize - 窗口大小改变时的回调
// 基类 D3DApp::OnResize() 负责重新创建渲染目标视图和深度/模板缓冲区
//---------------------------------------------------------
void GameApp::OnResize()
{
    D3DApp::OnResize();
}

//---------------------------------------------------------
// UpdateScene - 每帧调用，负责：
//   1. 更新物体旋转（用两个角度 phi, theta 控制 X 和 Y 轴旋转）
//   2. 渲染 ImGui 调试界面，让用户实时调整参数
//   3. 将 CPU 端修改的数据上传到 GPU 常量缓冲区
//---------------------------------------------------------
void GameApp::UpdateScene(float dt)
{
    if (ImGui::Begin("Texture Mapping"))
    {
        static int curr_mode_item = static_cast<int>(m_CurrMode);
        const char* mode_strs[] = {
            "Box",
            "Fire Anim",
            "Flare"
        };
        if (ImGui::Combo("Mode", &curr_mode_item, mode_strs, ARRAYSIZE(mode_strs)))
        {
            if (curr_mode_item == 0)
            {
                // 播放木箱动画
                m_CurrMode = ShowMode::WoodCrate;
                m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout3D.Get());
                auto meshData = Geometry::CreateBox();
                ResetMesh(meshData);
                m_pd3dImmediateContext->VSSetShader(m_pVertexShader3D.Get(), nullptr, 0);
                m_pd3dImmediateContext->PSSetShader(m_pPixelShader3D.Get(), nullptr, 0);
                // DrawScene 中会逐面绑定 m_pFaceSRV[face]，所以这里不绑定
            }
            else if (curr_mode_item == 1)
            {
                m_CurrMode = ShowMode::FireAnim;
                m_CurrFrame = 0;
                m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout2D.Get());
                auto meshData = Geometry::Create2DShow();
                ResetMesh(meshData);
                m_pd3dImmediateContext->VSSetShader(m_pVertexShader2D.Get(), nullptr, 0);
                m_pd3dImmediateContext->PSSetShader(m_pPixelShader2D.Get(), nullptr, 0);
                // 设置采样器
                m_pd3dImmediateContext->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());
                // 只需绑定一次纹理数组 SRV，后续通过 fireFrame 切换帧
                m_pd3dImmediateContext->PSSetShaderResources(0, 1, 
                    m_pFireAnimArray.GetAddressOf()     // 绑定到t0，也就是HLSL 的g_FireTex ，
                );
            }
            else
            {
                // Flare 模式：立方体 + 旋转纹理
                m_CurrMode = ShowMode::Flare;
                m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout3D.Get());
                auto meshData = Geometry::CreateBox();
                ResetMesh(meshData);
                m_pd3dImmediateContext->VSSetShader(m_pVertexShaderFlare.Get(), nullptr, 0);
                m_pd3dImmediateContext->PSSetShader(m_pPixelShaderFlare.Get(), nullptr, 0);
                // 绑定两个纹理：flare.dds (t0) 和 flarealpha.dds (t1)
                ID3D11ShaderResourceView* srvs[] = { m_pFlare.Get(), m_pFlareAlpha.Get() };
                m_pd3dImmediateContext->PSSetShaderResources(0, 2, srvs);
                // 使用 BORDER_COLOR 采样器
                m_pd3dImmediateContext->PSSetSamplers(0, 1, m_pSamplerBorderColor.GetAddressOf());
            }
        }
    }
    ImGui::End();
    ImGui::Render();

    if (m_CurrMode == ShowMode::WoodCrate)
    {
        static float phi = 0.0f, theta = 0.0f;
        phi += 0.0001f, theta += 0.00015f;
        XMMATRIX W = XMMatrixRotationX(phi) * XMMatrixRotationY(theta);
        m_VSConstantBuffer.world = XMMatrixTranspose(W);
        m_VSConstantBuffer.worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));

        // 更新常量缓冲区，让立方体转起来
        D3D11_MAPPED_SUBRESOURCE mappedData;
        HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[0].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
        memcpy_s(mappedData.pData, sizeof(VSConstantBuffer), &m_VSConstantBuffer, sizeof(VSConstantBuffer));
        m_pd3dImmediateContext->Unmap(m_pConstantBuffers[0].Get(), 0);
    }
    else if (m_CurrMode == ShowMode::FireAnim)
    {
        // 用于限制在1秒60帧
        static float totDeltaTime = 0;

        totDeltaTime += dt;
        if (totDeltaTime > 1.0f / 60)
        {
            totDeltaTime -= 1.0f / 60;
            m_CurrFrame = (m_CurrFrame + 1) % 120;

            // 更新 PS 常量缓冲区中的 fireFrame，HLSL 通过 g_FireFrame 选择纹理数组的层
            m_PSConstantBuffer.fireFrame = m_CurrFrame;
            D3D11_MAPPED_SUBRESOURCE mappedData;
            HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
            memcpy_s(mappedData.pData, sizeof(PSConstantBuffer), &m_PSConstantBuffer, sizeof(PSConstantBuffer));
            m_pd3dImmediateContext->Unmap(m_pConstantBuffers[1].Get(), 0);
        }		
    }
    else if (m_CurrMode == ShowMode::Flare)
    {
        // 立方体旋转
        static float phi = 0.0f, theta = 0.0f;
        phi += 0.0001f, theta += 0.00015f;
        XMMATRIX W = XMMatrixRotationX(phi) * XMMatrixRotationY(theta);
        m_VSConstantBuffer.world = XMMatrixTranspose(W);
        m_VSConstantBuffer.worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));

        D3D11_MAPPED_SUBRESOURCE mappedData;
        HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[0].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
        memcpy_s(mappedData.pData, sizeof(VSConstantBuffer), &m_VSConstantBuffer, sizeof(VSConstantBuffer));
        m_pd3dImmediateContext->Unmap(m_pConstantBuffers[0].Get(), 0);

        // 纹理旋转：绕 Z 轴旋转纹理坐标
        static float texAngle = 0.0f;
        texAngle += 0.5f * dt;  // 每秒旋转 0.5 弧度
        XMMATRIX texRot = XMMatrixRotationZ(texAngle);
        m_FlareConstantBuffer.texRotation = XMMatrixTranspose(texRot);

        HR(m_pd3dImmediateContext->Map(m_pFlareConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
        memcpy_s(mappedData.pData, sizeof(FlareConstantBuffer), &m_FlareConstantBuffer, sizeof(FlareConstantBuffer));
        m_pd3dImmediateContext->Unmap(m_pFlareConstantBuffer.Get(), 0);
    }
}

void GameApp::DrawScene()
{
    assert(m_pd3dImmediateContext);
    assert(m_pSwapChain);

    static const XMFLOAT4 bgColor(0.6f, 0.6f, 0.6f, 1.0f);
    m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&bgColor));
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    if (m_CurrMode == ShowMode::WoodCrate)
    {
        // 立方体：分6面绘制
        m_pd3dImmediateContext->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());
        for (int face = 0; face < 6; ++face)
        {
            // 这里srvs是数组的原因是，PSSetShaderResources需要传入一个数组，即使我们每次只绑定一个纹理资源视图，也必须传入一个长度为1的数组
            ID3D11ShaderResourceView* srvs[] = { m_pFaceSRV[face].Get() };
            m_pd3dImmediateContext->PSSetShaderResources(0, 1, srvs);
            m_pd3dImmediateContext->DrawIndexed(6, face * 6, 0);
            // 这里的face * 6是因为每个面有6个索引，face从0到5，依次绘制6个面，每个面使用不同的纹理资源视图。
            // 0~5，6~11，12~17，18~23，24~29，30~35，每6个索引对应一个面。
            // DrawIndexed的参数分别是：每个面绘制6个索引，索引从face * 6开始，顶点从0开始。
        }
    }
    else if (m_CurrMode == ShowMode::FireAnim)
    {
        // 全屏四边形：一次绘制
        // 纹理数组 SRV 和 fireFrame 常量已在 UpdateScene 中设置好
        m_pd3dImmediateContext->DrawIndexed(m_IndexCount, 0, 0);
    }
    else if (m_CurrMode == ShowMode::Flare)
    {
        // Flare 模式：立方体，所有面使用相同的两个纹理，一次绘制全部36个索引
        m_pd3dImmediateContext->DrawIndexed(36, 0, 0);
    }

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HR(m_pSwapChain->Present(0, 0));
}


bool GameApp::InitEffect()
{
    ComPtr<ID3DBlob> blob;

    // 创建顶点着色器(2D)
    HR(CreateShaderFromFile(L"HLSL\\Basic_2D_VS.cso", L"HLSL\\Basic_2D_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShader2D.GetAddressOf()));
    // 创建顶点布局(2D)
    HR(m_pd3dDevice->CreateInputLayout(VertexPosTex::inputLayout, ARRAYSIZE(VertexPosTex::inputLayout),
        blob->GetBufferPointer(), blob->GetBufferSize(), m_pVertexLayout2D.GetAddressOf()));

    // 创建像素着色器(2D)
    HR(CreateShaderFromFile(L"HLSL\\Basic_2D_PS.cso", L"HLSL\\Basic_2D_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShader2D.GetAddressOf()));

    // 创建顶点着色器(3D)
    HR(CreateShaderFromFile(L"HLSL\\Basic_3D_VS.cso", L"HLSL\\Basic_3D_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShader3D.GetAddressOf()));
    // 创建顶点布局(3D)
    HR(m_pd3dDevice->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
        blob->GetBufferPointer(), blob->GetBufferSize(), m_pVertexLayout3D.GetAddressOf()));

    // 创建像素着色器(3D)
    HR(CreateShaderFromFile(L"HLSL\\Basic_3D_PS.cso", L"HLSL\\Basic_3D_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShader3D.GetAddressOf()));

    // 创建顶点着色器(Flare)
    HR(CreateShaderFromFile(L"HLSL\\Flare_VS.cso", L"HLSL\\Flare_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShaderFlare.GetAddressOf()));

    // 创建像素着色器(Flare)
    HR(CreateShaderFromFile(L"HLSL\\Flare_PS.cso", L"HLSL\\Flare_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShaderFlare.GetAddressOf()));

    return true;
}

bool GameApp::InitResource()
{
    // 初始化网格模型并设置到输入装配阶段
    auto meshData = Geometry::CreateBox();
    ResetMesh(meshData);

    // ******************
    // 设置常量缓冲区描述
    //
    D3D11_BUFFER_DESC cbd;
    ZeroMemory(&cbd, sizeof(cbd));
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(VSConstantBuffer);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    // 新建用于VS和PS的常量缓冲区
    HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[0].GetAddressOf()));
    cbd.ByteWidth = sizeof(PSConstantBuffer);
    HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[1].GetAddressOf()));

    // 创建 Flare 纹理旋转矩阵常量缓冲区（寄存器 b2）
    cbd.ByteWidth = sizeof(FlareConstantBuffer);
    HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pFlareConstantBuffer.GetAddressOf()));

    // ******************
    // 初始化纹理和采样器状态
    //

    // 初始化木箱纹理
    // 使用 D3DX11 或 WICTextureLoader 加载纹理
    const wchar_t* faceFiles[6] = {
        L"Texture/1.png",   // +X
        L"Texture/2.png",    // -X
        L"Texture/3.png",     // +Y
        L"Texture/4.png",  // -Y
        L"Texture/5.png",    // +Z
        L"Texture/6.png"    // -Z
    };
    for (int i = 0; i < 6; ++i)
    {
        HR(CreateWICTextureFromFile(m_pd3dDevice.Get(), faceFiles[i],
            nullptr, m_pFaceSRV[i].GetAddressOf()));
    }

    // ******************
    // 创建火焰纹理数组 (Texture2DArray)
    // 将所有 120 帧 BMP 存储在一个纹理数组中
    //

    // 1. 加载第1帧获得纹理格式和尺寸
    ComPtr<ID3D11Resource> pFirstRes;
    ComPtr<ID3D11ShaderResourceView> pFirstSRV;
    HR(CreateWICTextureFromFile(m_pd3dDevice.Get(), L"Texture\\FireAnim\\Fire001.bmp",
        pFirstRes.GetAddressOf(), pFirstSRV.GetAddressOf()));

    ComPtr<ID3D11Texture2D> pFirstTex;
    HR(pFirstRes.As(&pFirstTex));
    D3D11_TEXTURE2D_DESC firstDesc;
    pFirstTex->GetDesc(&firstDesc);

    // 2. 创建 2D 纹理数组（120 层，每层一帧）
    D3D11_TEXTURE2D_DESC arrayDesc = {};
    arrayDesc.Width = firstDesc.Width;
    arrayDesc.Height = firstDesc.Height;
    arrayDesc.MipLevels = 1;
    arrayDesc.ArraySize = 120;
    arrayDesc.Format = firstDesc.Format;
    arrayDesc.SampleDesc.Count = 1;         // 不开启多重采样抗锯齿（MSAA）
    arrayDesc.Usage = D3D11_USAGE_DEFAULT;
    arrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ComPtr<ID3D11Texture2D> pFireAnimTex;
    HR(m_pd3dDevice->CreateTexture2D(&arrayDesc, nullptr, pFireAnimTex.GetAddressOf()));

    // 3. 将第1帧拷贝到数组的 slice 0
    m_pd3dImmediateContext->CopySubresourceRegion(
        pFireAnimTex.Get(), 0,  // dest, array slice 0 (subresource = 0)
        0, 0, 0,
        pFirstTex.Get(), 0,     // src, subresource 0
        nullptr);               // full copy

    // 4. 加载第2~120帧并拷贝到对应的 array slice
    for (int i = 2; i <= 120; ++i)
    {
        WCHAR strFile[40];
        wsprintf(strFile, L"Texture\\FireAnim\\Fire%03d.bmp", i);
        ComPtr<ID3D11Resource> pFrameRes;
        ComPtr<ID3D11ShaderResourceView> pFrameSRV;
        HR(CreateWICTextureFromFile(m_pd3dDevice.Get(), strFile,
            pFrameRes.GetAddressOf(), pFrameSRV.GetAddressOf()));

        ComPtr<ID3D11Texture2D> pFrameTex;
        HR(pFrameRes.As(&pFrameTex));

        // 拷贝到数组的第 (i-1) 个 slice
        // subresource = MipSlice + (ArraySlice * MipLevels) = 0 + (i-1) * 1 = i-1
        m_pd3dImmediateContext->CopySubresourceRegion(
            pFireAnimTex.Get(), i - 1,
            0, 0, 0,
            pFrameTex.Get(), 0,
            nullptr);
    }

    // 5. 为纹理数组创建 SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = arrayDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = 120;
    
    HR(m_pd3dDevice->CreateShaderResourceView(pFireAnimTex.Get(), &srvDesc, m_pFireAnimArray.GetAddressOf()));

    // 初始化 Flare 纹理（flare.dds 和 flarealpha.dds）
    HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\flare.dds",
        nullptr, m_pFlare.GetAddressOf()));
    HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\flarealpha.dds",
        nullptr, m_pFlareAlpha.GetAddressOf()));
        
    // 初始化采样器状态
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; 
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    HR(m_pd3dDevice->CreateSamplerState(&sampDesc, m_pSamplerState.GetAddressOf()));

    // 创建 BORDER_COLOR 寻址模式的采样器（用于 Flare 纹理旋转）
    D3D11_SAMPLER_DESC borderDesc;
    ZeroMemory(&borderDesc, sizeof(borderDesc));
    borderDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    borderDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    borderDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    borderDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    borderDesc.BorderColor[0] = 0.0f;  // R
    borderDesc.BorderColor[1] = 0.0f;  // G
    borderDesc.BorderColor[2] = 0.0f;  // B
    borderDesc.BorderColor[3] = 1.0f;  // A
    borderDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    borderDesc.MinLOD = 0;
    borderDesc.MaxLOD = D3D11_FLOAT32_MAX;
    HR(m_pd3dDevice->CreateSamplerState(&borderDesc, m_pSamplerBorderColor.GetAddressOf()));

    
    // ******************
    // 初始化常量缓冲区的值
    //

    // 初始化用于VS的常量缓冲区的值
    m_VSConstantBuffer.world = XMMatrixIdentity();			
    m_VSConstantBuffer.view = XMMatrixTranspose(XMMatrixLookAtLH(
        XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f),
        XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
    ));
    m_VSConstantBuffer.proj = XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV2, AspectRatio(), 1.0f, 1000.0f));
    m_VSConstantBuffer.worldInvTranspose = XMMatrixIdentity();
    
    // 初始化用于PS的常量缓冲区的值
    // 这里只使用一盏点光来演示
    m_PSConstantBuffer.pointLight[0].position = XMFLOAT3(0.0f, 0.0f, -10.0f);
    m_PSConstantBuffer.pointLight[0].ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
    m_PSConstantBuffer.pointLight[0].diffuse = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
    m_PSConstantBuffer.pointLight[0].specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    m_PSConstantBuffer.pointLight[0].att = XMFLOAT3(0.0f, 0.1f, 0.0f);
    m_PSConstantBuffer.pointLight[0].range = 25.0f;
    m_PSConstantBuffer.numDirLight = 0;
    m_PSConstantBuffer.numPointLight = 1;
    m_PSConstantBuffer.numSpotLight = 0;
    m_PSConstantBuffer.fireFrame = 0;   // 初始帧为第0帧
    // 初始化材质
    m_PSConstantBuffer.material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    m_PSConstantBuffer.material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    m_PSConstantBuffer.material.specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 5.0f);
    // 注意不要忘记设置此处的观察位置，否则高亮部分会有问题
    m_PSConstantBuffer.eyePos = XMFLOAT4(0.0f, 0.0f, -5.0f, 0.0f);

    // 更新PS常量缓冲区资源
    D3D11_MAPPED_SUBRESOURCE mappedData;
    HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
    memcpy_s(mappedData.pData, sizeof(PSConstantBuffer), &m_PSConstantBuffer, sizeof(PSConstantBuffer));
    m_pd3dImmediateContext->Unmap(m_pConstantBuffers[1].Get(), 0);

    // 初始化 Flare 纹理旋转矩阵（初始为单位矩阵）
    m_TexRotation = XMMatrixIdentity();
    m_FlareConstantBuffer.texRotation = XMMatrixTranspose(m_TexRotation);
    HR(m_pd3dImmediateContext->Map(m_pFlareConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
    memcpy_s(mappedData.pData, sizeof(FlareConstantBuffer), &m_FlareConstantBuffer, sizeof(FlareConstantBuffer));
    m_pd3dImmediateContext->Unmap(m_pFlareConstantBuffer.Get(), 0);

    // ******************
    // 给渲染管线各个阶段绑定好所需资源
    // 设置图元类型，设定输入布局
    m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout3D.Get());
    // 默认绑定3D着色器
    m_pd3dImmediateContext->VSSetShader(m_pVertexShader3D.Get(), nullptr, 0);
    // VS常量缓冲区对应HLSL寄存于b0的常量缓冲区
    m_pd3dImmediateContext->VSSetConstantBuffers(0, 1, m_pConstantBuffers[0].GetAddressOf());
    // PS常量缓冲区对应HLSL寄存于b1的常量缓冲区
    m_pd3dImmediateContext->PSSetConstantBuffers(1, 1, m_pConstantBuffers[1].GetAddressOf());
    // 像素着色阶段设置好采样器
    m_pd3dImmediateContext->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());
    // 默认绑定第1个面纹理；WoodCrate模式下DrawScene会逐面重新绑定
    m_pd3dImmediateContext->PSSetShaderResources(0, 1, m_pFaceSRV[0].GetAddressOf());
    m_pd3dImmediateContext->PSSetShader(m_pPixelShader3D.Get(), nullptr, 0);
    
    // Flare 模式默认绑定（b2 寄存器）
    m_pd3dImmediateContext->VSSetConstantBuffers(2, 1, m_pFlareConstantBuffer.GetAddressOf());
    m_pd3dImmediateContext->PSSetConstantBuffers(2, 1, m_pFlareConstantBuffer.GetAddressOf());
    
    // ******************
    // 设置调试对象名
    //
    D3D11SetDebugObjectName(m_pVertexLayout2D.Get(), "VertexPosTexLayout");
    D3D11SetDebugObjectName(m_pVertexLayout3D.Get(), "VertexPosNormalTexLayout");
    D3D11SetDebugObjectName(m_pConstantBuffers[0].Get(), "VSConstantBuffer");
    D3D11SetDebugObjectName(m_pConstantBuffers[1].Get(), "PSConstantBuffer");
    D3D11SetDebugObjectName(m_pVertexShader2D.Get(), "Basic_2D_VS");
    D3D11SetDebugObjectName(m_pVertexShader3D.Get(), "Basic_3D_VS");
    D3D11SetDebugObjectName(m_pPixelShader2D.Get(), "Basic_2D_PS");
    D3D11SetDebugObjectName(m_pPixelShader3D.Get(), "Basic_3D_PS");
    D3D11SetDebugObjectName(m_pSamplerState.Get(), "SSLinearWrap");
    D3D11SetDebugObjectName(m_pVertexShaderFlare.Get(), "Flare_VS");
    D3D11SetDebugObjectName(m_pPixelShaderFlare.Get(), "Flare_PS");
    D3D11SetDebugObjectName(m_pSamplerBorderColor.Get(), "SSBorder");
    D3D11SetDebugObjectName(m_pFlareConstantBuffer.Get(), "FlareConstantBuffer");

    return true;
}
template<class VertexType>
bool GameApp::ResetMesh(const Geometry::MeshData<VertexType>& meshData)
{
    // 释放旧资源
    m_pVertexBuffer.Reset();
    m_pIndexBuffer.Reset();



    // 设置顶点缓冲区描述
    D3D11_BUFFER_DESC vbd;
    ZeroMemory(&vbd, sizeof(vbd));
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = (UINT)meshData.vertexVec.size() * sizeof(VertexType);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    // 新建顶点缓冲区
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = meshData.vertexVec.data();
    HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.ReleaseAndGetAddressOf()));

    // 输入装配阶段的顶点缓冲区设置
    UINT stride = sizeof(VertexType);			// 跨越字节数
    UINT offset = 0;							// 起始偏移量

    m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);



    // 设置索引缓冲区描述
    m_IndexCount = (UINT)meshData.indexVec.size();
    D3D11_BUFFER_DESC ibd;
    ZeroMemory(&ibd, sizeof(ibd));
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(DWORD) * m_IndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    // 新建索引缓冲区
    InitData.pSysMem = meshData.indexVec.data();
    HR(m_pd3dDevice->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.ReleaseAndGetAddressOf()));
    // 输入装配阶段的索引缓冲区设置
    m_pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);



    // 设置调试对象名
    D3D11SetDebugObjectName(m_pVertexBuffer.Get(), "VertexBuffer");
    D3D11SetDebugObjectName(m_pIndexBuffer.Get(), "IndexBuffer");

    return true;
}