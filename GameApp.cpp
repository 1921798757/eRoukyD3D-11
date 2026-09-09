#include "GameApp.h"
#include <d3dUtil.h>
#include <DXTrace.h>
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D3DApp(hInstance, windowName, initWidth, initHeight),
    m_CameraMode(CameraMode::FirstPerson),
    m_CBFrame(),
    m_CBOnResize(),
    m_CBRarely()
{
}

GameApp::~GameApp()
{
}

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

void GameApp::OnResize()
{
    D3DApp::OnResize();

    // 摄像机变更显示
    if (m_pCamera != nullptr)
    {
        m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
        m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
        //为什么摄像机类非得要一份视口的数据？
        //显卡知道视口后，就只负责向显卡中填像素，GPU不会告诉CPU哪些像素被填充了，CPU也不需要知道这些信息。CPU只需要知道视口的大小和位置就可以了。
        //我们做部分计算时，CPU必须在无GPU参与的情况下独立完成坐标运算。所以必须将视口同步到Camera的成员变量中，方便CPU计算投影矩阵和视口变换矩阵。
        //1.比如做鼠标点击拾取时，当我们用鼠标点击某个像素，想要判断是否点击了场景中的木箱，
        //CPU拿到的是刚才的二维坐标，相机类利用刚刚SetViewPort存下的宽高，把像素坐标归一到NDC中，再利用投影矩阵和观察矩阵的逆矩阵，从相机眼睛
        //发射出一条穿越该像素的Ray，做射线与场景中物体的碰撞检测，判断是否点击了木箱。
        //2.世界坐标到屏幕坐标的投影（画血条等）
        //我们有敌人的3d世界坐标，想在屏幕上画一个2D UI血条，相机类通过矩阵变换将其乘到齐次裁剪空间，最后利用内部的ViewPort宽高，换算为屏幕上真实的像素坐标，画出血条。
        //D3DApp 是引擎底层，负责通知显卡怎么画；GameApp 是上层业务，负责通知摄像机数学模块参数已更新，算坐标时别算偏。



        //当窗口大小发生变化的时候，重新计算透视投影矩阵，并把这个新矩阵从CPU内存版于到显卡显存中的cb中
        //这个getprojxm()是获取透视投影矩阵的函数，返回的是一个xmmatrix类型的矩阵，这个矩阵的作用就是把摄像机看到的（一个金字塔削去顶部形状的）场景范围挤压变成
        //一个规则的长方体空间（NDC）
        //这里进行转置的原因是：
        //C++的XMMATRIX中，矩阵是行主序连续存储的，而HLSL中，矩阵是列主序连续存储的，所以在把矩阵从CPU传到GPU时，需要进行转置。
        m_CBOnResize.proj = XMMatrixTranspose(m_pCamera->GetProjXM());
        


        /*
        这段代码是D3D11中CPU向GPU传递数据最核心，最高校的标准操作。
        把存放在CPU内存中的投影矩阵m_CBOnResize.proj，传递到GPU显存中的常量缓冲区m_pConstantBuffers[2]中。
        */

        
        D3D11_MAPPED_SUBRESOURCE mappedData;
        //定义接收载体结构体，这是一个临时结构体，专门作为Map函数的输出参数，里面有一个void* pData指针，指向GPU显存中的常量缓冲区的首地址
        //那么为什么不直接返回一个void*指针，而是返回一个结构体呢？
        //如果我们只看到了常量缓冲区cb，它确实只用到了结构体里的void* pData指针，那么为啥不直接设计成void* Map(...)呢？
        //因为Map是一个通用API，它不仅用来映射一维的Buffer，还要用来映射Texture2D和3D纹理体积（Texture3D）
        /*
        typedef struct D3D11_MAPPED_SUBRESOURCE {
            void *pData;      // 内存起始地址指针
            UINT RowPitch;    // 【关键】每一行像素占用的实际字节数
            UINT DepthPitch;  // 3D 纹理中每一个切面占用的字节数
        } D3D11_MAPPED_SUBRESOURCE;
        */
        //为什么必须要有RowPitch?（考虑显卡的内存对齐）
        //假设一张宽度为100像素，每像素4字节(RGBA)的贴图：
        //逻辑上：一行应该是100*4=400字节
        //但在显卡硬件上：GPU为了快速读取，要求内存必须按照256字节（甚至更大）对齐，离400最近的256字节对齐是512字节，
        //所以显卡内部实际上给每行分配了512字节内存，后边的112字节是padding，无效。
        //此时如果拿一个裸的void*指针去按照400字节往里拷数据，整个贴图就会错乱，GPU读取的时候会把padding也当成像素数据读取，导致贴图错乱。
        //加深理解：
        //mappedData中的pData指针指向的是GPU显存中常量缓冲区的首地址，RowPitch是每行像素占用的实际字节数，DepthPitch是3D纹理中每一个切面占用的字节数。
        //拿一本书，100页，每页的每行80字来举例：
        //RowPitch 是跳到下一行所需的单行字节跨度，而 DepthPitch 是翻到下一页所需的整张切片纹理总大小。


        //Map和Unmap用于将显存/驱动管理的资源内存映射到CPU的虚拟地址空间，以便CPU直接读写。
        //其第二个参数Subresource含义是要指定映射资源的哪一个子资源，这里填0是因为：
        //DX11中，复制的纹理资源（比如带5级Mipmap，且包含6个面的立方体纹理数组）内部会细分为多个独立的子资源，每个Mip级别或数组切片就是一个子资源，需要用子资源索引来定位具体要映射哪一个。
        //Map和Unmap的subresource必须严格一致。
        //Map的第四个参数MapFlags的含义是，控制当GPU占用该资源时，CPU调用的等待行为。0表示CPU会阻塞等待GPU完成对该资源的访问，
        //D3D11_MAP_WRITE_DISCARD表示CPU写入时会丢弃原有数据，适用于动态更新的缓冲区。
        HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[2].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
        memcpy_s(mappedData.pData, sizeof(CBChangesOnResize), &m_CBOnResize, sizeof(CBChangesOnResize));
        m_pd3dImmediateContext->Unmap(m_pConstantBuffers[2].Get(), 0);
    }
}

void GameApp::UpdateScene(float dt)
{
    // 获取子类
    auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);
    auto cam3rd = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);

    Transform& woodCrateTransform = m_WoodCrate.GetTransform();

    ImGuiIO& io = ImGui::GetIO();
    if (m_CameraMode == CameraMode::FirstPerson || m_CameraMode == CameraMode::Free)
    {
        // 第一人称/自由摄像机的操作
        float d1 = 0.0f, d2 = 0.0f;
        if (ImGui::IsKeyDown(ImGuiKey_W))
            d1 += dt;
        if (ImGui::IsKeyDown(ImGuiKey_S))
            d1 -= dt;
        if (ImGui::IsKeyDown(ImGuiKey_A))
            d2 -= dt;
        if (ImGui::IsKeyDown(ImGuiKey_D))
            d2 += dt;

        if (m_CameraMode == CameraMode::FirstPerson)
            cam1st->Walk(d1 * 6.0f);
        else
            cam1st->MoveForward(d1 * 6.0f);
        cam1st->Strafe(d2 * 6.0f);//左右移动

        // 将摄像机位置限制在[-8.9, 8.9]x[-8.9, 8.9]x[0.0, 8.9]的区域内
        // 不允许穿地
        XMFLOAT3 adjustedPos;
        XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(), XMVectorSet(-8.9f, 0.0f, -8.9f, 0.0f), XMVectorReplicate(8.9f)));
        cam1st->SetPosition(adjustedPos);
        /*
        X 轴：截断在 [-8.9, 8.9] 之间。
        Y 轴：截断在 [0.0, 8.9] 之间（防止摄像机穿入地下或飞得过高）。
        Z 轴：截断在 [-8.9, 8.9]之间。
        W 通道：截断在 [0.0, 8.9] 之间（通常只关心 XYZ，W 不影响实际位置）。
        */

        // 仅在第一人称模式移动摄像机的同时移动箱子
        if (m_CameraMode == CameraMode::FirstPerson)
            woodCrateTransform.SetPosition(adjustedPos);

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            cam1st->Pitch(io.MouseDelta.y * 0.01f);
            cam1st->RotateY(io.MouseDelta.x * 0.01f);
        }
    }
    else if (m_CameraMode == CameraMode::ThirdPerson)
    {
        // 第三人称摄像机的操作
        cam3rd->SetTarget(woodCrateTransform.GetPosition());

        // 绕物体旋转
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            cam3rd->RotateX(io.MouseDelta.y * 0.01f);
            cam3rd->RotateY(io.MouseDelta.x * 0.01f);
        }
        cam3rd->Approach(-io.MouseWheel * 1.0f);
    }

    // 更新观察矩阵
    XMStoreFloat4(&m_CBFrame.eyePos, m_pCamera->GetPositionXM());
    m_CBFrame.view = XMMatrixTranspose(m_pCamera->GetViewXM());
    //获取摄像机当前的物理位置和视锥观察矩阵，并将矩阵转置后打包进CPU常量缓冲结构体m_CBFrame中，后续会传递到GPU显存的常量缓冲区中，供顶点着色器使用。
    //m_pCamera->GetViewXM()返回的是一个XMMATRIX类型的矩阵，表示摄像机的观察矩阵，它将世界坐标系中的点转换到摄像机的视图空间中。
    //这里转置的原因是因为HLSL中矩阵是列主序存储的，而C++中的XMMATRIX是行主序存储的，所以在传递给GPU之前需要进行转置。
    //为什么上方用XMStoreFloat4存储摄像机位置，而用m_CBFrame.view直接赋值观察矩阵？
    //因为m_CBFrame.eyePos是一个XMFLOAT4类型的变量，它是一个结构体，存储了摄像机的位置坐标（x, y, z, w），
    //而m_CBFrame.view是一个XMMATRIX类型的变量，它是一个矩阵，存储了摄像机的观察矩阵。XMMATRIX是一个特殊的类型，它在内存中是以列主序的方式存储的，
    //而XMFLOAT4是一个简单的结构体，它在内存中是以行主序的方式存储的。
    //DXMath中，计算类型的都是SIMD类型，存储类型的都是XMFLOAT类型。SIMD类型是为了高性能计算而设计的，它们通常是对齐的，
    //并且可以利用CPU的向量指令进行并行计算。而XMFLOAT类型是为了方便存储和传递数据而设计的，它们通常是非对齐的，并且不支持向量指令。
    //一般来说XMVECTOR和XMMATRIX类型的变量在内存中是16字节对齐的，而XMFLOAT3和XMFLOAT4类型的变量在内存中是非对齐的。
    //前者是SIMD类型，后者是存储类型。前者用于计算，后者用于存储和传递数据。
    //XMLoadFloat...:把RAM中的数据搬到CPU的SIMD寄存器中，进行计算
    //XMStoreFloat...:把CPU的SIMD寄存器中的数据搬到RAM中，进行存储
    //Get...:获取数据，返回的是CPU的SIMD寄存器中的数据，通常是XMVECTOR或XMMATRIX类型
    //
    //总结：
    //XMVECTOR和XMMATRIX是SIMD类型，用于高性能计算，通常是16字节对齐的；XMFLOAT3和XMFLOAT4是存储类型，用于存储和传递数据，通常是非对齐的。       


    if (ImGui::Begin("Camera"))
    {
        ImGui::Text("W/S/A/D in FPS/Free camera");
        ImGui::Text("Hold the right mouse button and drag the view");
        ImGui::Text("The box moves only at First Person mode");

        static int curr_item = 0;
        static const char* modes[] = {
            "First Person",
            "Third Person",
            "Free Camera"
        };
        if (ImGui::Combo("Camera Mode", &curr_item, modes, ARRAYSIZE(modes)))
        {
            //curr_item是UI层（当前瞬间用户的点击意图）。当在下拉框离选择了第一项时，ImGui把它改成0。
            //m_CameraMode是系统层（当前实际在运行的模式）。在代码还未执行m_CameraMode = CameraMode::FirstPerson;之前，它记录的是切换前的状态.
            if (curr_item == 0 && m_CameraMode != CameraMode::FirstPerson)
            {
                if (!cam1st)//cam1st未创建过，第一次切换到第一人称模式时，创建一个第一人称摄像机对象
                {
                    cam1st = std::make_shared<FirstPersonCamera>();
                    cam1st->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
                    m_pCamera = cam1st;
                }

                //LookTo函数的作用是设置摄像机的位置和朝向
                //LookAt(EyePos, TargetPos, Up)：
                //LookTo(EyePos, ToDir, Up):
                //区别在于，LookAt是通过指定目标点来确定摄像机的朝向，而LookTo是通过指定一个方向向量来确定摄像机的朝向。
                cam1st->LookTo(woodCrateTransform.GetPosition(),
                    XMFLOAT3(0.0f, 0.0f, 1.0f),
                    XMFLOAT3(0.0f, 1.0f, 0.0f));

                m_CameraMode = CameraMode::FirstPerson;
            }
            else if (curr_item == 1 && m_CameraMode != CameraMode::ThirdPerson)
            {
                if (!cam3rd)
                {
                    cam3rd = std::make_shared<ThirdPersonCamera>();
                    cam3rd->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
                    m_pCamera = cam3rd;
                }
                XMFLOAT3 target = woodCrateTransform.GetPosition();
                cam3rd->SetTarget(target);
                cam3rd->SetDistance(8.0f);
                cam3rd->SetDistanceMinMax(3.0f, 20.0f);

                m_CameraMode = CameraMode::ThirdPerson;
            }
            else if (curr_item == 2 && m_CameraMode != CameraMode::Free)
            {
                if (!cam1st)
                {
                    cam1st = std::make_shared<FirstPersonCamera>();
                    cam1st->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
                    m_pCamera = cam1st;
                }
                // 从箱子上方开始
                XMFLOAT3 pos = woodCrateTransform.GetPosition();
                XMFLOAT3 to = XMFLOAT3(0.0f, 0.0f, 1.0f);
                XMFLOAT3 up = XMFLOAT3(0.0f, 1.0f, 0.0f);
                pos.y += 3;
                cam1st->LookTo(pos, to, up);

                m_CameraMode = CameraMode::Free;
            }
        }
        auto woodPos = woodCrateTransform.GetPosition();
        ImGui::Text("Box Position\n%.2f %.2f %.2f", woodPos.x, woodPos.y, woodPos.z);
        auto cameraPos = m_pCamera->GetPosition();
        ImGui::Text("Camera Position\n%.2f %.2f %.2f", cameraPos.x, cameraPos.y, cameraPos.z);
    }
    ImGui::End();   //End()必须在Begin()之后调用，End表示，这个具体窗口的内容塞完了，如果一帧中有三个不同的窗口，那么就必须有三个Begin()和三个End()，且顺序必须一一对应，不能乱套。
    ImGui::Render();//Render()必须在End()之后调用，Render表示，这一帧的所有窗口都塞完了，ImGui开始计算每个窗口的绘制数据，
                    //最终生成一个ImDrawData结构体，里面包含了所有窗口的绘制命令和顶点数据。
                    //ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData())
                    //真正向 DirectX 11 发送绘制命令（DrawIndexed），把 UI 渲染到屏幕上。

    D3D11_MAPPED_SUBRESOURCE mappedData;
    HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
    memcpy_s(mappedData.pData, sizeof(CBChangesEveryFrame), &m_CBFrame, sizeof(CBChangesEveryFrame));
    m_pd3dImmediateContext->Unmap(m_pConstantBuffers[1].Get(), 0);
}

void GameApp::DrawScene()
{
    assert(m_pd3dImmediateContext);
    assert(m_pSwapChain);

    m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    //
    // 绘制几何模型
    //
    m_WoodCrate.Draw(m_pd3dImmediateContext.Get());
    m_Floor.Draw(m_pd3dImmediateContext.Get());
    for (auto& wall : m_Walls)
        wall.Draw(m_pd3dImmediateContext.Get());

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HR(m_pSwapChain->Present(0, 0));
}


bool GameApp::InitEffect()
{
    //可以把它简单理解成 Direct3D 提供的通用纯二进制内存块（Buffer）
    //无论是读取已经编译好的着色器文件（.cso，Compiled Shader Object），
    //还是动态编译文本文件（.hlsl），编译器输出的结果都是一堆原始二进制字节（Bytecode）。blob 就是用来临时装这堆字节的指针容器
    ComPtr<ID3DBlob> blob;  //Binary Large Object，二进制大对象

    // 创建顶点着色器(2D)
    HR(CreateShaderFromFile(L"HLSL\\Basic_2D_VS.cso", L"HLSL\\Basic_2D_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShader2D.GetAddressOf()));
    // 创建顶点布局(2D)
    HR(m_pd3dDevice->CreateInputLayout(VertexPosTex::inputLayout, ARRAYSIZE(VertexPosTex::inputLayout),
        blob->GetBufferPointer(), blob->GetBufferSize(), m_pVertexLayout2D.GetAddressOf()));
    // 这个ARRAYSIZE的作用是在编译期计算出静态原生数组的元素个数。它的实现原理是：sizeof(数组)/sizeof(数组元素类型)，所以它只能用于静态原生数组，不能用于动态数组和STL容器。

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

    return true;
}

bool GameApp::InitResource()
{
    // ******************
    // 设置常量缓冲区描述
    D3D11_BUFFER_DESC cbd;
    ZeroMemory(&cbd, sizeof(cbd));
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    // 新建用于VS和PS的常量缓冲区
    // 后边四个cb除了bytewidth不同，其他描述都是一样的
    cbd.ByteWidth = sizeof(CBChangesEveryDrawing);
    HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[0].GetAddressOf()));
    cbd.ByteWidth = sizeof(CBChangesEveryFrame);
    HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[1].GetAddressOf()));
    cbd.ByteWidth = sizeof(CBChangesOnResize);
    HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[2].GetAddressOf()));
    cbd.ByteWidth = sizeof(CBChangesRarely);
    HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[3].GetAddressOf()));
    
    // ******************
    // 初始化游戏对象
    ComPtr<ID3D11ShaderResourceView> texture;
    // 初始化木箱
    HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\WoodCrate.dds", nullptr, texture.GetAddressOf()));
    m_WoodCrate.SetBuffer(m_pd3dDevice.Get(), Geometry::CreateBox());
    m_WoodCrate.SetTexture(texture.Get());
    
    // 初始化地板
    HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\floor.dds", nullptr, texture.ReleaseAndGetAddressOf()));
    m_Floor.SetBuffer(m_pd3dDevice.Get(),
        Geometry::CreatePlane(XMFLOAT2(20.0f, 20.0f), XMFLOAT2(5.0f, 5.0f)));
    m_Floor.SetTexture(texture.Get());
    m_Floor.GetTransform().SetPosition(0.0f, -1.0f, 0.0f);
    
    
    // 初始化墙体,resize是vector的成员函数，作用是改变容器中元素的个数
    m_Walls.resize(4);
    HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"..\\Texture\\brick.dds", nullptr, texture.ReleaseAndGetAddressOf()));
    // 这里控制墙体四个面的生成
    for (int i = 0; i < 4; ++i)
    {
        m_Walls[i].SetBuffer(m_pd3dDevice.Get(),
            Geometry::CreatePlane(XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 1.5f)));
        Transform& transform = m_Walls[i].GetTransform();
        transform.SetRotation(-XM_PIDIV2, XM_PIDIV2 * i, 0.0f);
        transform.SetPosition(i % 2 ? -10.0f * (i - 2) : 0.0f, 3.0f, i % 2 == 0 ? -10.0f * (i - 1) : 0.0f);
        m_Walls[i].SetTexture(texture.Get());
    }
        
    // 初始化采样器状态
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  //三线性过滤
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     //对应x轴                       这是寻址模式，参数规定了纹理坐标超出[0,1]范围时的处理方式。
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;     //对应y轴                       D3D11_TEXTURE_ADDRESS_WRAP表示纹理坐标会在[0,1]范围内循环重复。
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;     //对应z轴
    /*
    CLAMP（边缘截断）：超出部分永远取边缘最后那一个像素的颜色（拉伸边缘）。
    MIRROR（镜像翻转）：超出后像照镜子一样反向翻转重复。
    BORDER（边框色）：超出部分直接返回指定的纯色（配合 BorderColor 使用）。
    */

    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;   //决定当前采样的值是否要与某个参考值（Reference Value）做大小比较。
    /*
    如果以后做 阴影映射（Shadow Mapping / PCF 软阴影），
    会把采样器设为比较模式（如 D3D11_COMPARISON_LESS_EQUAL），此时硬件会自动对比当前深度与阴影图中的深度，并直接输出阴影遮挡比例。
    */


    //LOD（Level of Detail）是指纹理的细节层次，LOD 越低，纹理越清晰，LOD 越高，纹理越模糊。LOD 的值通常是一个浮点数，表示当前采样的纹理级别。
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    HR(m_pd3dDevice->CreateSamplerState(&sampDesc, m_pSamplerState.GetAddressOf()));

    
    // ******************
    // 初始化常量缓冲区的值
    // 初始化每帧可能会变化的值
    m_CameraMode = CameraMode::FirstPerson;
    auto camera = std::make_shared<FirstPersonCamera>();    //std::make_shared在堆上创建一个对象，并返回一个指向该对象的std::shared_ptr智能指针。
                                                            //它会自动管理对象的生命周期，当最后一个shared_ptr被销毁时，对象会被自动释放。
                                                            //这里不用COM的原因是这个camera内有跨二进制边界的需求，dx使用COM，是因为COM是微软预编译好的系统DLL，
                                                            //不管上层游戏是用MSVC,Clang编译，还是用C#，Rust调用，只要符合COM规范，就可以跨二进制边界调用。
                                                            //COM的引用计数机制是微软预编译号的系统DLL提供的，跨二进制边界调用时，引用计数机制是安全的。
    m_pCamera = camera;
    camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    //这里交换链的buffer和视口viewport解决的是两个不同层面的几何映射问题
    //比如交换链的buffer是一个800x600的纹理，它决定了显存里的这一块用来接像素的二维数组总共有多少个像素点。
    //而视口是投影贴图的一个窗口，它决定了渲染管线输出的像素点在屏幕上显示的区域大小和位置。视口可以小于、等于或大于交换链的buffer大小。会有缩放和裁剪的效果。
    //GPU顶点着色器计算完毕之后，顶点三维坐标会被压到NDC之中，dx的标准是，x和y坐标在[-1,1]之间，z坐标在[0,1]之间。然后GPU会把NDC坐标映射到屏幕坐标，
    //屏幕坐标的范围是[0, width]和[0, height]，这个映射过程就是视口变换。视口变换会把NDC的[-1,1]范围映射到屏幕坐标的[0,width]和[0,height]范围。
    //而屏幕大小就是交换链的buffer大小。视口变换的公式是：
    // screenX = (NDCx + 1) * 0.5 * width
    // screenY = (1 - NDCy) * 0.5 * height
    // 其中width和height就是交换链的buffer的宽度和高度。

    camera->LookAt(XMFLOAT3(), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

    // 初始化仅在窗口大小变动时修改的值
    m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
    m_CBOnResize.proj = XMMatrixTranspose(m_pCamera->GetProjXM());

    // 初始化不会变化的值
    // 环境光
    m_CBRarely.dirLight[0].ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    m_CBRarely.dirLight[0].diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    m_CBRarely.dirLight[0].specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    m_CBRarely.dirLight[0].direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
    // 灯光
    m_CBRarely.pointLight[0].position = XMFLOAT3(0.0f, 10.0f, 0.0f);
    m_CBRarely.pointLight[0].ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    m_CBRarely.pointLight[0].diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
    m_CBRarely.pointLight[0].specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    m_CBRarely.pointLight[0].att = XMFLOAT3(0.0f, 0.1f, 0.0f);
    m_CBRarely.pointLight[0].range = 25.0f;
    m_CBRarely.numDirLight = 1;
    m_CBRarely.numPointLight = 1;
    m_CBRarely.numSpotLight = 0;
    // 初始化材质
    m_CBRarely.material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    m_CBRarely.material.diffuse = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
    m_CBRarely.material.specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 50.0f);


    // 更新不容易被修改的常量缓冲区资源
    D3D11_MAPPED_SUBRESOURCE mappedData;
    HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[2].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
    memcpy_s(mappedData.pData, sizeof(CBChangesOnResize), &m_CBOnResize, sizeof(CBChangesOnResize));
    m_pd3dImmediateContext->Unmap(m_pConstantBuffers[2].Get(), 0);

    HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[3].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
    memcpy_s(mappedData.pData, sizeof(CBChangesRarely), &m_CBRarely, sizeof(CBChangesRarely));
    m_pd3dImmediateContext->Unmap(m_pConstantBuffers[3].Get(), 0);

    // ******************
    // 给渲染管线各个阶段绑定好所需资源
    // 设置图元类型，设定输入布局
    m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout3D.Get());
    // 默认绑定3D着色器
    m_pd3dImmediateContext->VSSetShader(m_pVertexShader3D.Get(), nullptr, 0);
    // 预先绑定各自所需的缓冲区，其中每帧更新的缓冲区需要绑定到两个缓冲区上
    m_pd3dImmediateContext->VSSetConstantBuffers(0, 1, m_pConstantBuffers[0].GetAddressOf());
    m_pd3dImmediateContext->VSSetConstantBuffers(1, 1, m_pConstantBuffers[1].GetAddressOf());
    m_pd3dImmediateContext->VSSetConstantBuffers(2, 1, m_pConstantBuffers[2].GetAddressOf());

    m_pd3dImmediateContext->PSSetConstantBuffers(1, 1, m_pConstantBuffers[1].GetAddressOf());
    m_pd3dImmediateContext->PSSetConstantBuffers(3, 1, m_pConstantBuffers[3].GetAddressOf());
    m_pd3dImmediateContext->PSSetShader(m_pPixelShader3D.Get(), nullptr, 0);
    m_pd3dImmediateContext->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());

    // ******************
    // 设置调试对象名
    //
    D3D11SetDebugObjectName(m_pVertexLayout2D.Get(), "VertexPosTexLayout");
    D3D11SetDebugObjectName(m_pVertexLayout3D.Get(), "VertexPosNormalTexLayout");
    D3D11SetDebugObjectName(m_pConstantBuffers[0].Get(), "CBDrawing");
    D3D11SetDebugObjectName(m_pConstantBuffers[1].Get(), "CBFrame");
    D3D11SetDebugObjectName(m_pConstantBuffers[2].Get(), "CBOnResize");
    D3D11SetDebugObjectName(m_pConstantBuffers[3].Get(), "CBRarely");
    D3D11SetDebugObjectName(m_pVertexShader2D.Get(), "Basic_2D_VS");
    D3D11SetDebugObjectName(m_pVertexShader3D.Get(), "Basic_3D_VS");
    D3D11SetDebugObjectName(m_pPixelShader2D.Get(), "Basic_2D_PS");
    D3D11SetDebugObjectName(m_pPixelShader3D.Get(), "Basic_3D_PS");
    D3D11SetDebugObjectName(m_pSamplerState.Get(), "SSLinearWrap");
    m_Floor.SetDebugObjectName("Floor");
    m_WoodCrate.SetDebugObjectName("WoodCrate");
    m_Walls[0].SetDebugObjectName("Walls[0]");
    m_Walls[1].SetDebugObjectName("Walls[1]");
    m_Walls[2].SetDebugObjectName("Walls[2]");
    m_Walls[3].SetDebugObjectName("Walls[3]");


    return true;
}

GameApp::GameObject::GameObject()
    : m_IndexCount(), m_VertexStride()
{
}

Transform& GameApp::GameObject::GetTransform()
{
    return m_Transform;
}

const Transform& GameApp::GameObject::GetTransform() const
{
    return m_Transform;
}

template<class VertexType, class IndexType>
void GameApp::GameObject::SetBuffer(ID3D11Device * device, const Geometry::MeshData<VertexType, IndexType>& meshData)
{
    // 释放旧资源
    m_pVertexBuffer.Reset();
    m_pIndexBuffer.Reset();

    // 设置顶点缓冲区描述
    m_VertexStride = sizeof(VertexType);
    D3D11_BUFFER_DESC vbd;
    ZeroMemory(&vbd, sizeof(vbd));
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = (UINT)meshData.vertexVec.size() * m_VertexStride;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    // 新建顶点缓冲区
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = meshData.vertexVec.data();
    HR(device->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.GetAddressOf()));


    // 设置索引缓冲区描述
    m_IndexCount = (UINT)meshData.indexVec.size();
    D3D11_BUFFER_DESC ibd;
    ZeroMemory(&ibd, sizeof(ibd));
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = m_IndexCount * sizeof(IndexType);
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    // 新建索引缓冲区
    InitData.pSysMem = meshData.indexVec.data();
    HR(device->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.GetAddressOf()));
}

void GameApp::GameObject::SetTexture(ID3D11ShaderResourceView * texture)
{
    m_pTexture = texture;
}

void GameApp::GameObject::Draw(ID3D11DeviceContext * deviceContext)
{
    // 设置顶点/索引缓冲区
    UINT strides = m_VertexStride;
    UINT offsets = 0;
    deviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &strides, &offsets);
    deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    // 获取之前已经绑定到渲染管线上的常量缓冲区并进行修改
    ComPtr<ID3D11Buffer> cBuffer = nullptr;
    deviceContext->VSGetConstantBuffers(0, 1, cBuffer.GetAddressOf());
    CBChangesEveryDrawing cbDrawing;

    // 内部进行转置
    XMMATRIX W = m_Transform.GetLocalToWorldMatrixXM();
    cbDrawing.world = XMMatrixTranspose(W);
    cbDrawing.worldInvTranspose = XMMatrixTranspose(InverseTranspose(W));

    // 更新常量缓冲区
    D3D11_MAPPED_SUBRESOURCE mappedData;
    HR(deviceContext->Map(cBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
    memcpy_s(mappedData.pData, sizeof(CBChangesEveryDrawing), &cbDrawing, sizeof(CBChangesEveryDrawing));
    deviceContext->Unmap(cBuffer.Get(), 0);

    // 设置纹理
    deviceContext->PSSetShaderResources(0, 1, m_pTexture.GetAddressOf());
    // 可以开始绘制
    deviceContext->DrawIndexed(m_IndexCount, 0, 0);
}

void GameApp::GameObject::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    D3D11SetDebugObjectName(m_pVertexBuffer.Get(), name + ".VertexBuffer");
    D3D11SetDebugObjectName(m_pIndexBuffer.Get(), name + ".IndexBuffer");
#else
    UNREFERENCED_PARAMETER(name);
#endif
}
