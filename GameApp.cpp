//===========================================================
// GameApp.cpp - 游戏主程序实现
// 
// 这个版本的主程序专注于"光照渲染"演示：
//   - 使用 ImGui 提供实时调试界面
//   - 可以切换 4 种网格模型（立方体、球体、圆柱体、圆锥体）
//   - 可以切换 3 种光源类型（方向光、点光源、聚光灯）
//   - 可以实时调整材质颜色和光照参数
//   - 支持线框模式
//===========================================================

#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D3DApp(hInstance, windowName, initWidth, initHeight), 
    m_IndexCount(),             // 初始化为0，后面InitResource会设置具体值
    m_VSConstantBuffer(),       // 初始化为零
    m_PSConstantBuffer(),       // 初始化为零
    m_DirLight(),               // 方向光初始化为零
    m_PointLight(),             // 点光源初始化为零
    m_SpotLight(),              // 聚光灯初始化为零
    m_FillMode(0),              // 默认 Solid 模式
    m_CullMode(1),              // 默认 Back Cull
    m_ShowTriangleEdges(false)  // 默认不叠加三角形边界
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
    // 先初始化 D3DApp 基类（创建设备、交换链、渲染目标等）
    if (!D3DApp::Init())
        return false;

    // 加载/编译着色器
    if (!InitEffect())
        return false;

    // 初始化网格数据、常量缓冲区、光照参数等
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
    //=====================================================
    // 第一部分：更新物体变换矩阵（让物体自动旋转）
    //=====================================================
    
    // phi 和 theta 是两个旋转角度，每帧递增
    // dt 是帧间隔时间（单位：秒），确保旋转速度与帧率无关
    // 0.3 和 0.37 是旋转速度系数，数值越大转得越快
    static float phi = 0.0f, theta = 0.0f;
    phi += 0.3f * dt, theta += 0.37f * dt;
    
    // 构造旋转矩阵：先绕 X 轴旋转 phi 角度，再绕 Y 轴旋转 theta 角度
    // 这样物体会同时绕两个轴旋转，产生翻滚效果
    XMMATRIX W = XMMatrixRotationX(phi) * XMMatrixRotationY(theta);
    
    // 为什么需要转置？
    // HLSL 默认使用列主矩阵（column-major），而 DirectXMath 使用行主矩阵（row-major）
    // 所以从 CPU 传到 GPU 时需要转置，否则矩阵会出错
    // 后续的所有矩阵都做同样处理（view, proj 也一样）
    m_VSConstantBuffer.world = XMMatrixTranspose(W);
    
    // 计算世界矩阵的逆转置矩阵，用于正确变换法向量
    // 当有非均匀缩放时（如拉伸模型），法向量不能直接用世界矩阵变换
    // InverseTranspose() 是 DirectX 工具函数，计算 (W⁻¹)ᵀ
    m_VSConstantBuffer.worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));

    //=====================================================
    // 第二部分：ImGui 调试界面
    // 
    // ImGui 是什么？
    //   ImGui（Dear ImGui）是一个即时模式 GUI 库
    //   它每一帧都重新绘制 UI，不需要像传统 GUI 那样维护复杂的消息循环
    //   非常适合用于游戏/图形调试工具的界面
    //
    // 为什么用 ImGui？
    //   我们可以实时调整光照参数，立即看到效果变化
    //   而不需要修改代码重新编译
    //=====================================================
    
    // ImGui::Begin("标题") 开始一个窗口，ImGui::End() 结束
    // 在 Begin/End 之间的代码就是窗口的内容
    if (ImGui::Begin("Lighting"))
    {
        // ---- (1) 网格类型选择 ComboBox ----
        // curr_mesh_item 是当前选中的项索引（0=Box, 1=Sphere, ...）
        static int curr_mesh_item = 0;
        const char* mesh_strs[] = {
            "Box",      // 立方体
            "Sphere",   // 球体
            "Cylinder", // 圆柱体
            "Cone",     // 圆锥体
            "Capsule"   // 胶囊体
        };
        // ImGui::Combo("标签", &变量, 选项数组, 选项数量)
        // 当用户选择了不同的项时返回 true
        if (ImGui::Combo("Mesh", &curr_mesh_item, mesh_strs, ARRAYSIZE(mesh_strs)))
        {
            // 用户选择了不同的网格，重新生成几何体数据
            Geometry::MeshData<VertexPosNormalColor> meshData;
            switch (curr_mesh_item)
            {
            case 0: meshData = Geometry::CreateBox<VertexPosNormalColor>(); break;
            case 1: meshData = Geometry::CreateSphere<VertexPosNormalColor>(); break;
            case 2: meshData = Geometry::CreateCylinder<VertexPosNormalColor>(); break;
            case 3: meshData = Geometry::CreateCone<VertexPosNormalColor>(); break;
            case 4: meshData = Geometry::CreateCapsule<VertexPosNormalColor>(); break;
            }
            // 重置缓冲区，用新的网格数据替换旧的
            ResetMesh(meshData);
        }

        // ---- (2) 材质颜色编辑 ----
        // ImGui::Text() 显示纯文本（用作分组标签）
        ImGui::Text("Material");
        // ImGui::PushID(ID) / PopID() 用于创建唯一的控件 ID
        // 因为下面有多个 "Ambient"/"Diffuse"/"Specular" 控件（材质和光源共用同名控件），
        // 需要不同的 ID 来区分它们，否则 ImGui 会混淆
        ImGui::PushID(3);
        // ColorEdit3(label, float[3]) 显示一个 RGB 颜色编辑器（三个 float 输入框 + 色块）
        // 第三个参数类型是 float*，指向 3 个连续 float 的起始地址
        // Material::ambient/diffuse/specular 都是 XMFLOAT4（成员为 x,y,z,w）
        // &xxx.x 就是 &xxx，即指向 XMFLOAT4 起始位置的 float*
        // ColorEdit3 把它当 float[3] 处理，只读写偏移 0,1,2 的位置 = .x(R), .y(G), .z(B)
        // .w 分量不受影响（ambient.w/diffuse.w=1.0, specular.w=镜面强度由hlsl常量指定）
        // 这些修改直接写入 CPU 内存，然后在 UpdateScene() 末尾 Map+memcpy 上传到 GPU
        //
        // 【为什么 UI 拖动修改后可能会被"弹回"原值？】
        // ColorEdit3 是直接通过指针写内存的，所以拖动时 UI 和内存都会变。
        // 但如果拖动后 UI 又回到原来的值，说明在 ColorEdit3 执行之后，
        // UpdateScene() 中还有别的代码又修改了 m_PSConstantBuffer.material 的同一字段。
        // 常见的"回写"场景：
        //   1. ColorEdit3 之后有一个 Combo("Light Type") 回调，其中做了
        //      m_PSConstantBuffer.material = 某个预设值 → 覆盖了前面的修改
        //   2. Combo("Mesh") 回调中调用了 ResetMesh，ResetMesh 内部重新设置了材质
        //   3. 切换光源类型时，某个预设光源赋值给 PSConstantBuffer 时
        //      顺带覆盖了 material 字段（但当前代码仅覆盖 dirLight/pointLight/spotLight）
        //   4. 用户代码中某个 if 分支每帧都在重置材质（如 m_PSConstantBuffer.material.ambient = ...）
        // 解决方法：找到 UpdateScene() 中在 ColorEdit3 之后修改 material 的那行代码，去掉它。
        // 因为 ColorEdit3 已经在修改 m_PSConstantBuffer.material 的内存了，
        // 后面不需要（也不应该）再对这个材质做任何赋值。
        ImGui::ColorEdit3("Ambient", &m_PSConstantBuffer.material.ambient.x);   // 环境光反射色 (RGB -> XMFLOAT4.x/.y/.z)
        ImGui::ColorEdit3("Diffuse", &m_PSConstantBuffer.material.diffuse.x);   // 漫反射色（物体的主要颜色）(RGB -> .x/.y/.z)
        ImGui::ColorEdit3("Specular", &m_PSConstantBuffer.material.specular.x); // 镜面高光色 (RGB -> .x/.y/.z)
        ImGui::PopID();
        
        //====================================
        //作业4：调整镜面强度（观看镜面强度<1的现象）
        //====================================
        // 找到 ImGui 中设置材质颜色的区域，在后面添加：
        ImGui::Text("Material Specular Power");
        ImGui::SliderFloat("Specular Power", &m_PSConstantBuffer.material.specular.w, 0.1f, 128.0f);

        // ---- (3) 光源类型选择 ComboBox ----
        // curr_light_item 记录当前选择的光源类型索引
        static int curr_light_item = 0;
        static const char* light_modes[] = {
            "Directional Light",  // 方向光（平行光），索引0
            "Point Light",        // 点光源，索引1
            "Spot Light"          // 聚光灯，索引2
        };
        ImGui::Text("Light");
        if (ImGui::Combo("Light Type", &curr_light_item, light_modes, ARRAYSIZE(light_modes)))
        {
            // 切换光源时的逻辑：
            // 如果选择了某种光源，就把预设的光照参数赋值给 PSConstantBuffer
            // 没选中的光源设为默认值（黑色 = 不发光）
            // 这样一次只有一个光源生效，便于观察每种光源的效果
            m_PSConstantBuffer.dirLight = (curr_light_item == 0 ? m_DirLight : DirectionalLight());
            m_PSConstantBuffer.pointLight = (curr_light_item == 1 ? m_PointLight : PointLight());
            m_PSConstantBuffer.spotLight = (curr_light_item == 2 ? m_SpotLight : SpotLight());
        }

        // ---- (4) 根据光源类型显示对应的参数编辑控件 ----
        // light_changed 目前没有实际使用（可以是以后扩展的标志）
        // 用 PushID(curr_light_item) 区分不同光源类型的同名控件
        ImGui::PushID(curr_light_item);
        if (curr_light_item == 0)
        {
            // 方向光参数：只有颜色可调，没有位置/衰减（方向光是无限远的）
            ImGui::ColorEdit3("Ambient", &m_PSConstantBuffer.dirLight.ambient.x);
            ImGui::ColorEdit3("Diffuse", &m_PSConstantBuffer.dirLight.diffuse.x);
            ImGui::ColorEdit3("Specular", &m_PSConstantBuffer.dirLight.specular.x);
        }
        else if (curr_light_item == 1)
        {
            // 点光源参数：颜色 + 范围 + 衰减系数
            ImGui::ColorEdit3("Ambient", &m_PSConstantBuffer.pointLight.ambient.x);
            ImGui::ColorEdit3("Diffuse", &m_PSConstantBuffer.pointLight.diffuse.x);
            ImGui::ColorEdit3("Specular", &m_PSConstantBuffer.pointLight.specular.x);
            ImGui::InputFloat("Range", &m_PSConstantBuffer.pointLight.range);       // 照射范围
            ImGui::InputFloat3("Attenutation", &m_PSConstantBuffer.pointLight.att.x); // 衰减系数(A0,A1,A2)
        }
        else
        {
            // 聚光灯参数：颜色 + 汇聚指数 + 范围 + 衰减系数
            ImGui::ColorEdit3("Ambient", &m_PSConstantBuffer.spotLight.ambient.x);
            ImGui::ColorEdit3("Diffuse", &m_PSConstantBuffer.spotLight.diffuse.x);
            ImGui::ColorEdit3("Specular", &m_PSConstantBuffer.spotLight.specular.x);
            ImGui::InputFloat("Spot", &m_PSConstantBuffer.spotLight.spot);     // 汇聚指数（越大光锥越窄）
            ImGui::InputFloat("Range", &m_PSConstantBuffer.spotLight.range);   // 照射范围
            ImGui::InputFloat3("Attenutation", &m_PSConstantBuffer.spotLight.att.x); // 衰减系数
        }
        ImGui::PopID();

        // ---- (5) 渲染模式控制 ----
        ImGui::Text("Fill Mode:");
        ImGui::RadioButton("Solid", &m_FillMode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Wireframe", &m_FillMode, 1);

        ImGui::Text("Cull Mode:");
        ImGui::RadioButton("None", &m_CullMode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Back", &m_CullMode, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Front", &m_CullMode, 2);

        // 独立checkbox：是否叠加三角形边界线
        ImGui::Checkbox("Show Triangle Edges", &m_ShowTriangleEdges);
    }
    ImGui::End();           // 结束 ImGui 窗口
    ImGui::Render();        // 生成 ImGui 绘制命令（实际渲染在 DrawScene 中执行）


    //=====================================================
    // 作业部分：强制灯光颜色和材质颜色
    //
    // 【注意】这个代码块位于 ImGui::ColorEdit3 之后、GPU 上传之前，
    // 所以它会每帧覆盖掉用户在 UI 中拖拽修改的值。
    //
    // 执行顺序：
    //   1. ColorEdit3 修改了 m_PSConstantBuffer.material/dirLight/... 等字段
    //      （用户拖拽色块 → 写内存，UI 看到的是用户刚拖的值）
    //   2. ↓ 执行到这里，用下面的 XMFLOAT4(...) 重新赋值为固定值
    //      （把 ColorEdit3 写进去的值又覆盖掉了）
    //   3. ↓ Map+memcpy 上传到 GPU
    //      （GPU 拿到的是被覆盖后的固定值，不是用户拖的值）
    //
    // 结果：用户拖拽后 UI 控件会立刻"弹回"到这些固定值，
    //       因为每帧在 ColorEdit3 之后又被重新赋值为固定颜色。
    //
    // 如果想让 UI 拖拽生效，需要：
    //   - 把这整个"作业部分"注释掉，或移到 ColorEdit3 之前，
    //     或放到只在首次运行时执行的 InitResource() 中。
    //======================================================
    // 题目1：强制方向光为红光
    /*
    m_PSConstantBuffer.dirLight.ambient = XMFLOAT4(0.2f, 0.0f, 0.0f, 1.0f);
    m_PSConstantBuffer.dirLight.diffuse = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    m_PSConstantBuffer.dirLight.specular = XMFLOAT4(0.5f, 0.0f, 0.0f, 1.0f);

    // 题目1：强制点光源为绿光
    m_PSConstantBuffer.pointLight.ambient = XMFLOAT4(0.0f, 0.3f, 0.0f, 1.0f);
    m_PSConstantBuffer.pointLight.diffuse = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
    m_PSConstantBuffer.pointLight.specular = XMFLOAT4(0.0f, 0.5f, 0.0f, 1.0f);

    // 题目1：强制聚光灯为蓝光
    m_PSConstantBuffer.spotLight.ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    m_PSConstantBuffer.spotLight.diffuse = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
    m_PSConstantBuffer.spotLight.specular = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

    // 题目2：强制材质只反射红光
    m_PSConstantBuffer.material.ambient = XMFLOAT4(0.5f, 0.0f, 0.0f, 1.0f);
    m_PSConstantBuffer.material.diffuse = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    m_PSConstantBuffer.material.specular = XMFLOAT4(0.5f, 0.0f, 0.0f, 5.0f);
    */
    //=====================================================
    // 作业部分3：
    // 
    //=====================================================
    // 鼠标滚轮控制聚光灯汇聚强度
    short wheelDelta = GetMouseWheelDelta();
    if (wheelDelta != 0)
    {
        // WHEEL_DELTA = 120，每滚动一格调整 2 个单位
        float delta = (float)wheelDelta / WHEEL_DELTA * 2.0f;
        m_PSConstantBuffer.spotLight.spot += delta;
        
        // 限制范围 2-512
        if (m_PSConstantBuffer.spotLight.spot < 2.0f)
            m_PSConstantBuffer.spotLight.spot = 2.0f;
        if (m_PSConstantBuffer.spotLight.spot > 512.0f)
            m_PSConstantBuffer.spotLight.spot = 512.0f;
    }

    //=====================================================
    // 第三部分：将 CPU 数据上传到 GPU 常量缓冲区
    //
    // Map/Discard/Unmap 模式解释：
    //   D3D11_MAP_WRITE_DISCARD 告诉 GPU "我要丢弃旧数据写入新数据"
    //   这避免了 CPU 等待 GPU 读取完成（不会阻塞管线）
    //   每次 Map 都获得一块新内存，不会影响正在渲染的那一帧
    //=====================================================
    
    D3D11_MAPPED_SUBRESOURCE mappedData;
    
    // 上传 VS 常量缓冲区（寄存器 b0）：世界矩阵、观察矩阵等
    HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[0].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
    memcpy_s(mappedData.pData, sizeof(VSConstantBuffer), &m_VSConstantBuffer, sizeof(VSConstantBuffer));
    m_pd3dImmediateContext->Unmap(m_pConstantBuffers[0].Get(), 0);

    // 上传 PS 常量缓冲区（寄存器 b1）：光照参数、材质、摄像机位置等
    HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
    memcpy_s(mappedData.pData, sizeof(PSConstantBuffer), &m_PSConstantBuffer, sizeof(PSConstantBuffer));
    m_pd3dImmediateContext->Unmap(m_pConstantBuffers[1].Get(), 0);
}

//---------------------------------------------------------
// DrawScene - 每帧绘制场景
// 流程：清屏 → 绘制几何体 → 绘制ImGui → 交换缓冲区显示
//---------------------------------------------------------
void GameApp::DrawScene()
{
    assert(m_pd3dImmediateContext);
    assert(m_pSwapChain);

    // 清除渲染目标为黑色（背景色）
    m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
    // 清除深度/模板缓冲区（1.0f表示最远，这样所有像素都会通过深度测试）
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    
    // 根据 ImGui 的 FillMode 选择绘制方式
    if (m_FillMode == 0)
    {
        // Solid 模式：画实体立方体
        m_pd3dImmediateContext->RSSetState(m_pRS[0 * 3 + m_CullMode].Get());
        m_pd3dImmediateContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
        m_pd3dImmediateContext->DrawIndexed(m_IndexCount, 0, 0);
    }
    else
    {
        // Wireframe 模式：画三角形线框
        m_pd3dImmediateContext->RSSetState(m_pRS[1 * 3 + m_CullMode].Get());
        m_pd3dImmediateContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
        m_pd3dImmediateContext->DrawIndexed(m_IndexCount, 0, 0);
    }

    // 独立功能：叠加三角形边界线（不影响 Solid/Wireframe 切换）
    if (m_ShowTriangleEdges)
    {
        // 使用 LESS_EQUAL 深度比较，让相同深度的边线也能通过深度测试
        // 使用固定白色像素着色器，让边线在实体/线框上清晰可见
        m_pd3dImmediateContext->RSSetState(m_pRS[1 * 3 + m_CullMode].Get());
        m_pd3dImmediateContext->OMSetDepthStencilState(m_pDSEqual.Get(), 0);
        m_pd3dImmediateContext->PSSetShader(m_pWireframePS.Get(), nullptr, 0);
        m_pd3dImmediateContext->DrawIndexed(m_IndexCount, 0, 0);
    }

    // 渲染 ImGui 的绘制命令（在 UpdateScene 中已经生成了绘制数据）
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Present(同步间隔, 标志)：将后台缓冲区显示到屏幕上
    // 参数 0,0 表示不等待垂直同步（不锁定帧率）
    HR(m_pSwapChain->Present(0, 0));
}

//---------------------------------------------------------
// InitEffect - 加载/编译着色器
// 
// 流程：
//   1. 从 .hlsl 源文件编译顶点着色器和像素着色器
//   2. 从顶点着色器的字节码创建输入布局（告诉GPU顶点数据的格式）
//   3. 编译好的 .cso 文件是已经编译好的着色器二进制文件
//      如果 .cso 不存在，CreateShaderFromFile 会自动从 .hlsl 编译生成
//---------------------------------------------------------
bool GameApp::InitEffect()
{
    ComPtr<ID3DBlob> blob;  // 存储着色器编译后的字节码

    // ---- 创建顶点着色器 ----
    // 参数：.cso路径, .hlsl源文件路径, 入口函数名, 着色器模型版本
    // "vs_5_0" 表示使用 Vertex Shader 5.0（对应 Direct3D 11）
    HR(CreateShaderFromFile(L"HLSL\\Light_VS.cso", L"HLSL\\Light_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShader.GetAddressOf()));
    
    // ---- 创建顶点输入布局 ----
    // 从顶点着色器的字节码中解析输入布局
    // 这告诉 GPU：顶点数据由 position(float3)、normal(float3)、color(float4) 组成
    // 这些信息定义在 VertexPosNormalColor::inputLayout 中
    HR(m_pd3dDevice->CreateInputLayout(VertexPosNormalColor::inputLayout, ARRAYSIZE(VertexPosNormalColor::inputLayout),
        blob->GetBufferPointer(), blob->GetBufferSize(), m_pVertexLayout.GetAddressOf()));

    // ---- 创建像素着色器 ----
    // "ps_5_0" 表示 Pixel Shader 5.0
    HR(CreateShaderFromFile(L"HLSL\\Light_PS.cso", L"HLSL\\Light_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShader.GetAddressOf()));

    // ---- 创建线框像素着色器（输出固定白色，用于叠加三角形边界） ----
    HR(CreateShaderFromFile(L"HLSL\\Wireframe_PS.cso", L"HLSL\\Wireframe_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pWireframePS.GetAddressOf()));

    return true;
}

//---------------------------------------------------------
// InitResource - 初始化所有渲染资源
// 
// 初始化内容：
//   1. 创建默认的立方体网格数据
//   2. 创建 VS 和 PS 的常量缓冲区（D3D11_USAGE_DYNAMIC 表示每帧更新）
//   3. 初始化三种光源的默认参数
//   4. 设置观察矩阵（摄像机位置在 (0,0,-5) 看向原点）
//   5. 设置投影矩阵（45° 视场角，透视投影）
//   6. 创建线框模式的光栅化状态
//   7. 将所有资源绑定到渲染管线
//---------------------------------------------------------
bool GameApp::InitResource()
{
    //=====================================================
    // 第1步：创建默认网格模型（立方体）
    //=====================================================
    // Geometry::CreateBox 生成一个立方体的顶点和索引数据
    // 顶点格式是 VertexPosNormalColor，包含位置、法向量和颜色
    auto meshData = Geometry::CreateBox<VertexPosNormalColor>();
    ResetMesh(meshData);  // 创建顶点缓冲区和索引缓冲区

    //=====================================================
    // 第2步：创建常量缓冲区（Constant Buffer）
    // 
    // 常量缓冲区是 CPU -> GPU 的数据通道
    // D3D11_USAGE_DYNAMIC 允许 CPU 每帧更新数据
    // D3D11_CPU_ACCESS_WRITE 允许 CPU 写入
    //=====================================================
    D3D11_BUFFER_DESC cbd;
    ZeroMemory(&cbd, sizeof(cbd));
    cbd.Usage = D3D11_USAGE_DYNAMIC;      // 动态使用（CPU每帧写入）
    cbd.ByteWidth = sizeof(VSConstantBuffer);  // 大小和CPU端结构体一致
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // 标志：常量缓冲区
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU可写
    // 创建 VS 常量缓冲区（寄存器 b0）
    HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[0].GetAddressOf()));
    cbd.ByteWidth = sizeof(PSConstantBuffer);
    // 创建 PS 常量缓冲区（寄存器 b1）
    HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[1].GetAddressOf()));

    //=====================================================
    // 第3步：初始化三种光源的默认参数
    // 
    // 方向光（Directional Light）：
    //   模拟太阳，光线方向固定，没有位置概念，强度不随距离衰减
    //   这里设置的光方向 (-0.577, -0.577, 0.577) 是一个单位化向量
    //   表示从"左前上"方向照射
    //=====================================================
    
    // ---- 方向光 ----
    m_DirLight.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);   // 环境光：微弱灰色
    m_DirLight.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);   // 漫反射：亮白色
    m_DirLight.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);  // 镜面高光：中灰色
    m_DirLight.direction = XMFLOAT3(-0.577f, -0.577f, 0.577f); // 从左上方向右下方照射
    
    // ---- 点光源 ----
    m_PointLight.position = XMFLOAT3(0.0f, 0.0f, -10.0f);     // 位于物体前方（靠近摄像机）
    m_PointLight.ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
    m_PointLight.diffuse = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
    m_PointLight.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    m_PointLight.att = XMFLOAT3(0.0f, 0.1f, 0.0f);            // 衰减系数：只有线性衰减(A1=0.1)
    m_PointLight.range = 25.0f;                                 // 范围：25个单位
    
    // ---- 聚光灯 ----
    m_SpotLight.position = XMFLOAT3(0.0f, 0.0f, -5.0f);       // 位于物体正前方
    m_SpotLight.direction = XMFLOAT3(0.0f, 0.0f, 1.0f);       // 朝 Z 正方向照射
    m_SpotLight.ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);  // 无环境光（观察效果更明显）
    m_SpotLight.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);  // 纯白色漫反射
    m_SpotLight.specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // 纯白色高光
    m_SpotLight.att = XMFLOAT3(1.0f, 0.0f, 0.0f);             // 衰减：只有常数项(不衰减)
    m_SpotLight.spot = 12.0f;                                   // 汇聚指数：12（中等锥角）
    m_SpotLight.range = 10000.0f;                               // 范围：非常大（几乎无距离限制）

    //=====================================================
    // 第4步：设置摄像机（观察矩阵和投影矩阵）
    //=====================================================
    
    // 世界矩阵：初始化为单位矩阵（不进行任何变换）
    m_VSConstantBuffer.world = XMMatrixIdentity();
    
    // 观察矩阵：摄像机在 (0,0,-5) 看向原点 (0,0,0)
    // XMMatrixLookAtLH 是左手坐标系下的观察矩阵
    // 参数：摄像机位置, 目标点, 上方向
    // 需要转置因为 HLSL 使用列主矩阵
    m_VSConstantBuffer.view = XMMatrixTranspose(XMMatrixLookAtLH(
        XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f),  // 摄像机位置
        XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),   // 看向原点
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)    // 上方向（Y轴向上）
    ));
    
    // 投影矩阵：透视投影
    // XM_PIDIV2 = 90° 视场角（实际是 45° 因为此函数用的是半角？不对，是完整视场角90°）
    // AspectRatio() 是窗口宽高比
    // 近裁剪面 1.0, 远裁剪面 1000.0
    m_VSConstantBuffer.proj = XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV2, AspectRatio(), 1.0f, 1000.0f));
    m_VSConstantBuffer.worldInvTranspose = XMMatrixIdentity();  // 逆转置矩阵初始化为单位矩阵

    //=====================================================
    // 第5步：设置材质属性和默认光源
    //=====================================================
    
    // 材质属性
    m_PSConstantBuffer.material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);   // 环境光反射率
    m_PSConstantBuffer.material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);   // 漫反射率（白色，显示光源本身的颜色）
    m_PSConstantBuffer.material.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 5.0f);  // 镜面反射率（w=5 中等高光）
    
    // 默认使用方向光
    m_PSConstantBuffer.dirLight = m_DirLight;
    // 设置摄像机位置（用于计算镜面高光）
    m_PSConstantBuffer.eyePos = XMFLOAT4(0.0f, 0.0f, -5.0f, 0.0f);

    // 将 PS 常量缓冲区数据上传到 GPU
    D3D11_MAPPED_SUBRESOURCE mappedData;
    HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
    memcpy_s(mappedData.pData, sizeof(PSConstantBuffer), &m_PSConstantBuffer, sizeof(PSConstantBuffer));
    m_pd3dImmediateContext->Unmap(m_pConstantBuffers[1].Get(), 0);

    //=====================================================
    // 第6步：创建 6 种光栅化状态组合
    //=====================================================
    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZeroMemory(&rasterizerDesc, sizeof(rasterizerDesc));
    rasterizerDesc.FrontCounterClockwise = false;        // 逆时针为正面
    rasterizerDesc.DepthClipEnable = true;

    // [0] Solid + None
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    HR(m_pd3dDevice->CreateRasterizerState(&rasterizerDesc, m_pRS[0].GetAddressOf()));

    // [1] Solid + Back
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    HR(m_pd3dDevice->CreateRasterizerState(&rasterizerDesc, m_pRS[1].GetAddressOf()));

    // [2] Solid + Front
    rasterizerDesc.CullMode = D3D11_CULL_FRONT;
    HR(m_pd3dDevice->CreateRasterizerState(&rasterizerDesc, m_pRS[2].GetAddressOf()));

    // [3] Wireframe + None
    rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    HR(m_pd3dDevice->CreateRasterizerState(&rasterizerDesc, m_pRS[3].GetAddressOf()));

    // [4] Wireframe + Back
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    HR(m_pd3dDevice->CreateRasterizerState(&rasterizerDesc, m_pRS[4].GetAddressOf()));

    // [5] Wireframe + Front
    rasterizerDesc.CullMode = D3D11_CULL_FRONT;
    HR(m_pd3dDevice->CreateRasterizerState(&rasterizerDesc, m_pRS[5].GetAddressOf()));

    //=====================================================
    // 第6.5步：创建深度模板状态（LESS_EQUAL，用于第二遍wireframe叠加）
    // 默认深度比较是 LESS（严格小于），第二遍 wireframe 的边线深度与第一遍 solid 相同，
    // 会被深度测试剔除。改用 LESS_EQUAL 让相同深度的像素也能通过。
    //=====================================================
    D3D11_DEPTH_STENCIL_DESC dsDesc;
    ZeroMemory(&dsDesc, sizeof(dsDesc));
    dsDesc.DepthEnable = true;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;  // 小于等于都通过
    HR(m_pd3dDevice->CreateDepthStencilState(&dsDesc, m_pDSEqual.GetAddressOf()));

    //=====================================================
    // 第7步：将资源绑定到渲染管线
    //=====================================================

    // 设置图元类型为三角形列表
    m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    // 绑定输入布局
    m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout.Get());
    // 绑定顶点着色器
    m_pd3dImmediateContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
    // 绑定 VS 常量缓冲区到寄存器 b0
    m_pd3dImmediateContext->VSSetConstantBuffers(0, 1, m_pConstantBuffers[0].GetAddressOf());
    // 绑定 PS 常量缓冲区到寄存器 b1
    m_pd3dImmediateContext->PSSetConstantBuffers(1, 1, m_pConstantBuffers[1].GetAddressOf());
    // 绑定像素着色器
    m_pd3dImmediateContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

    // ---- 设置调试对象名（方便 Visual Studio 的图形调试工具查看） ----
    D3D11SetDebugObjectName(m_pVertexLayout.Get(), "VertexPosNormalTexLayout");
    D3D11SetDebugObjectName(m_pConstantBuffers[0].Get(), "VSConstantBuffer");
    D3D11SetDebugObjectName(m_pConstantBuffers[1].Get(), "PSConstantBuffer");
    D3D11SetDebugObjectName(m_pVertexShader.Get(), "Light_VS");
    D3D11SetDebugObjectName(m_pPixelShader.Get(), "Light_PS");

    return true;
}

//---------------------------------------------------------
// ResetMesh - 重置网格数据
// 
// 作用：当用户在 ImGui 中切换到不同的网格类型时，
//       用新的顶点/索引数据替换旧的缓冲区
//
// 参数 meshData: 包含顶点数组和索引数组的网格数据
//
// 顶点缓冲区（Vertex Buffer）：存储所有顶点（位置、法向量、颜色）
// 索引缓冲区（Index Buffer）：存储顶点索引，指定绘制顺序
//   使用索引缓冲区可以避免存储重复的顶点（节省内存）
//---------------------------------------------------------
bool GameApp::ResetMesh(const Geometry::MeshData<VertexPosNormalColor>& meshData)
{
    // 释放旧的缓冲区资源（ComPtr 的 Reset 自动释放引用）
    m_pVertexBuffer.Reset();
    m_pIndexBuffer.Reset();

    //=====================================================
    // 重建顶点缓冲区
    // D3D11_USAGE_IMMUTABLE：创建后不可修改（因为网格数据在运行时不变）
    //=====================================================
    D3D11_BUFFER_DESC vbd;
    ZeroMemory(&vbd, sizeof(vbd));
    vbd.Usage = D3D11_USAGE_IMMUTABLE;   // 不可变（创建后只读，最高效）
    vbd.ByteWidth = (UINT)meshData.vertexVec.size() * sizeof(VertexPosNormalColor);
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;              // CPU不可访问（GPU只读）
    
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = meshData.vertexVec.data();  // 指向顶点数据的指针
    HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.GetAddressOf()));

    // 绑定顶点缓冲区到输入装配阶段（IA，Input Assembler）
    UINT stride = sizeof(VertexPosNormalColor);	// 每个顶点占用的字节数
    UINT offset = 0;							// 从缓冲区开头开始读取
    m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);

    //=====================================================
    // 重建索引缓冲区
    //=====================================================
    m_IndexCount = (UINT)meshData.indexVec.size();
    D3D11_BUFFER_DESC ibd;
    ZeroMemory(&ibd, sizeof(ibd));
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = m_IndexCount * sizeof(DWORD);  // DWORD = unsigned long (32位)
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    
    InitData.pSysMem = meshData.indexVec.data();
    HR(m_pd3dDevice->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.GetAddressOf()));
    // 绑定索引缓冲区到 IA 阶段
    // DXGI_FORMAT_R32_UINT：每个索引是 32 位无符号整数
    m_pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    // 设置调试对象名
    D3D11SetDebugObjectName(m_pVertexBuffer.Get(), "VertexBuffer");
    D3D11SetDebugObjectName(m_pIndexBuffer.Get(), "IndexBuffer");

    return true;
}
