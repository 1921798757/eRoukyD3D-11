// ------------------------------
// @file GameApp.cpp
// @eRouky
// GameApp类的实现文件，包含了游戏应用的初始化、资源加载、场景更新和绘制等功能
// 继承自D3DApp类，重载了父类的虚函数以实现具体的游戏逻辑和渲染细节
// ------------------------------
// 
//
#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

const D3D11_INPUT_ELEMENT_DESC GameApp::VertexPosColor::inputLayout[2] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },   // POSITION语义，表示顶点位置，格式是3个32位浮点数，偏移量为0
    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }   // COLOR语义，表示顶点颜色，格式是4个32位浮点数，偏移量为12（因为前面有一个XMFLOAT3，占用12字节）
};                                                                                          // 这里POSITION和COLOR是我们在HLSL着色器代码中定义的输入语义，必须与之对应，GPU才能正确地将顶点数据传递给着色器。     
// 参数说明：语义名称、语义索引、数据格式、输入槽、起始偏移量、输入数据分类、实例数据步长
// 1.语义名称：对应HLSL代码     float4 color : COLOR;等
// 2.语义索引：当有多个相同语义时用来区分，例如：float4 color0 : COLOR0; float4 color1 : COLOR1;
// 3.数据格式：DXGI_FORMAT_R32G32B32A32_FLOAT表示4个32位浮点数，DXGI_FORMAT_R32G32B32_FLOAT表示3个32位浮点数
// 4.输入槽：当使用多个顶点缓冲区时用来区分，这里我们只有一个顶点缓冲区，所以设置为0；显卡有 0~15 共 16 个输入槽可以同时挂载不同的顶点缓冲区。我们刚刚绑在第 0 号槽。
// 5.起始偏移量：表示该属性在每个顶点数据中的字节偏移量，POSITION在结构体的开头，所以是0，COLOR在POSITION之后，所以是12字节
// 6.输入数据分类：“更新频率”。通常填D3D11_INPUT_PER_VERTEX_DATA，表示每个顶点都有一套数据；意思是这是按照“每个顶点”来读取数据的，而不是按照“每个实例”来读取数据的。对于我们这个简单的立方体来说，每个顶点都有自己的位置和颜色数据，所以我们使用D3D11_INPUT_PER_VERTEX_DATA。
// 7.实例数据步长：当输入数据分类为D3D11_INPUT_PER_INSTANCE_DATA时，表示每个实例数据的字节大小，这里我们不使用实例数据，所以设置为0,
// “实例化绘画”专用，只有当用一个模型画一万片不同位置的树叶（硬件实例化技术）时才会用到，平时画普通物体永远填0

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D3DApp(hInstance, windowName, initWidth, initHeight)
{
}

GameApp::~GameApp()
{
}

// ------------------------------
// GameApp::Init函数
// ------------------------------
// App的初始化
// 返回值: true
// 返回值: false
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
}

void GameApp::UpdateScene(float dt)
{
    float t = m_Timer.TotalTime();  // 获取从Reset()调用之后经过的时间，但不包括暂停期间的时间，这个时间值可以用来驱动动画，使得动画的速度与帧率无关。

    // =====================1.静态变换（矩阵算好存起来）======================
    // 直接用总时间t乘以速度来计算绝对角度，这样无论帧率如何变化，旋转的速度都是恒定的。
    XMMATRIX R = XMMatrixRotationX(0.3f * t) * XMMatrixRotationY(0.37f * t);   // 通过绕X轴旋转0.3倍的时间t和绕Y轴旋转0.37倍的时间t的矩阵乘积，得到一个综合的旋转矩阵
    XMStoreFloat4x4(&m_BaseRotation, R);    // 将旋转矩阵存储到m_BaseRotation成员变量中，这个变量会在DrawScene函数中用来设置世界矩阵，从而实现旋转效果

    // =====================2.动态形变（捏顶点的颜色）======================
    // 遍历所有13个顶点，根据时间t动态修改它们的颜色属性，使得立方体和四棱锥的颜色会随着时间变化而产生动态的视觉效果。
    for(int i =0;i<13;i++)
    {
        // 加入顶点索引 i 作为相位差 (例如 i * 0.5f)
    // 这样每个顶点在同一时刻的颜色都不一样，就能恢复漂亮的渐变和 3D 立体感！
        m_Vertices[i].color.x = (sin(t * 3.0f + i * 0.5f) + 1.0f) / 2.0f; 
    
        // 蓝色通道也加入不同的相位差
        m_Vertices[i].color.z = (cos(t * 2.0f + i * 0.8f) + 1.0f) / 2.0f;
    }

    // =====================3.显存更新（发送给顶点缓冲区）======================
    D3D11_MAPPED_SUBRESOURCE mappedData;    // 准备一个D3D11_MAPPED_SUBRESOURCE结构体来接收映射后的内存信息
    // 注意这里的map是m_pVertexBuffer，而不是m_pConstantBuffer，因为我们要更新的是顶点数据，而不是常量缓冲区的数据
    HR(m_pd3dImmediateContext->Map(m_pVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));   // 将顶点缓冲区映射到CPU可访问的内存中，准备写入数据
    memcpy_s(mappedData.pData, sizeof(m_Vertices), m_Vertices, sizeof(m_Vertices));   // 将修改后的顶点数据复制到映射后的内存中，这样GPU就能在渲染时使用这些新的顶点数据了
    m_pd3dImmediateContext->Unmap(m_pVertexBuffer.Get(), 0);   // 解除映射，告诉GPU我们已经完成了对顶点缓冲区的更新，可以使用新的数据进行渲染了
}

void GameApp::DrawScene()
{
    assert(m_pd3dImmediateContext);
    assert(m_pSwapChain);

    static float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };	// RGBA = (0,0,0,255)
    m_pd3dImmediateContext->ClearRenderTargetView(      // 清除渲染目标视图，填充为黑色
        m_pRenderTargetView.Get(),                  
        reinterpret_cast<const float*>(&black)          // &black不是 float ，也不是float* ,如果你直接写black，会退化成一个首元素地址指针，float*,
                                                        // &black（对数组取地址），虽然它在内存里指向的起始位置和用black一样，但在cpp 编译器中，
                                                        // 它被视为一个指向整个数组的指针，类型是float(*)[4]，而不是float*。所以需要用reinterpret_cast<const float*>(&black)来告诉编译器我们确实想要一个float*类型的指针。
    );
    m_pd3dImmediateContext->ClearDepthStencilView(
        m_pDepthStencilView.Get(),
         D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 
         1.0f,      // 这是给深度缓冲区（Z-Buffer）设定的初始值。
         0);        // 这是给模板缓冲区（Stencil Buffer）设定的初始值。

    
    

    XMMATRIX baseRot = XMLoadFloat4x4(&m_BaseRotation);   // 从m_BaseRotation成员变量中加载旋转矩阵到一个XMMATRIX类型的变量baseRot中，这样我们就可以在GPU上使用这个旋转矩阵来变换顶点位置，实现立方体的旋转效果。
    D3D11_MAPPED_SUBRESOURCE mappedData;    // 准备一个D3D11_MAPPED_SUBRESOURCE结构体来接收映射后的内存信息
    // 左边的四棱锥(左移3m)
    m_CBuffer.world = XMMatrixTranspose(baseRot * XMMatrixTranslation(-3.0f, 0.0f, 0.0f));   // world矩阵是一个组合矩阵，包含了基础旋转和一个平移变换，使得四棱锥位于立方体的左侧
    // 打包给显卡
    HR(m_pd3dImmediateContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));   // 将常量缓冲区映射到CPU可访问的内存中，准备写入数据
    memcpy_s(mappedData.pData, sizeof(m_CBuffer), &m_CBuffer, sizeof(m_CBuffer));   // 将C++中的常量缓冲区数据复制到映射后的内存中，这样GPU就能在渲染时使用这些数据了
    m_pd3dImmediateContext->Unmap(m_pConstantBuffer.Get(), 0);   // 解除映射，告诉GPU我们已经完成了对常量缓冲区的更新，可以使用新的数据进行渲染了
    // 绘制四棱锥
    m_pd3dImmediateContext->DrawIndexed(18, 36, 0);


    // 右边的立方体(右移3m)
    m_CBuffer.world = XMMatrixTranspose(baseRot * XMMatrixTranslation(3.0f, 0.0f, 0.0f));    // world矩阵是一个组合矩阵，包含了基础旋转和一个平移变换，使得立方体位于四棱锥的右侧
    // 打包给显卡
    HR(m_pd3dImmediateContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));   // 将常量缓冲区映射到CPU可访问的内存中，准备写入数据
    memcpy_s(mappedData.pData, sizeof(m_CBuffer), &m_CBuffer, sizeof(m_CBuffer));   // 将C++中的常量缓冲区数据复制到映射后的内存中，这样GPU就能在渲染时使用这些数据了
    m_pd3dImmediateContext->Unmap(m_pConstantBuffer.Get(), 0);   // 绘制立方体
    m_pd3dImmediateContext->DrawIndexed(36, 0, 0);
    HR(m_pSwapChain->Present(0, 0));
}

// ------------------------------
// GameApp::InitEffect函数
// ------------------------------
// 初始化着色器和输入布局
// 返回值: true
// 返回值: false
bool GameApp::InitEffect()
{
    ComPtr<ID3DBlob> blob;// 用于存储编译后的着色器字节码，Binary Large Object，二进制大对象,只是一块单纯的内存缓冲区。

    // 创建顶点着色器
    HR(CreateShaderFromFile(L"HLSL\\Cube_VS.cso", L"HLSL\\Cube_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShader.GetAddressOf()));
    // 创建顶点布局
    HR(m_pd3dDevice->CreateInputLayout(VertexPosColor::inputLayout, ARRAYSIZE(VertexPosColor::inputLayout),
        blob->GetBufferPointer(), blob->GetBufferSize(), m_pVertexLayout.GetAddressOf()));

    // 创建像素着色器
    HR(CreateShaderFromFile(L"HLSL\\Cube_PS.cso", L"HLSL\\Cube_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShader.GetAddressOf()));

    return true;
}

bool GameApp::InitResource()
{
    // ******************
    // 设置立方体顶点
    //    5________ 6
    //    /|      /|
    //   /_|_____/ |
    //  1|4|_ _ 2|_|7
    //   | /     | /
    //   |/______|/
    //  0       3
    VertexPosColor vertices[] =
    {
        // 立方体顶点
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },    // 0
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },     // 1
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },      // 2
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },     // 3
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },     // 4
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },      // 5
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },       // 6
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },      // 7

        // 四棱锥顶点
        { XMFLOAT3(-1.0f, -0.5f, -1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },    // 8
        { XMFLOAT3(0.0f, 3.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },         // 9
        { XMFLOAT3(-1.0f, -0.5f, 1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },      // 10
        { XMFLOAT3(1.0f, -0.5f, 1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },       // 11
        { XMFLOAT3(1.0f, -0.5f, -1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }        // 12

    };
    // 设置顶点缓冲区描述,vbd是Vertex Buffer Description的缩写
    D3D11_BUFFER_DESC vbd;
    ZeroMemory(&vbd, sizeof(vbd));  
    vbd.Usage = D3D11_USAGE_DYNAMIC;            // 从IMMUTABLE改为DYNAMIC，因为我们需要在每一帧更新常量缓冲区的数据，而顶点数据虽然不需要频繁修改，但为了代码的统一和简化，我们也设置为DYNAMIC，这样就可以使用同样的映射方式来更新顶点数据了。
    vbd.ByteWidth = sizeof(vertices);            // 该缓冲区的大小，单位是字节。这里我们直接用整个顶点数组的大小来设置
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU访问权限，D3D11_CPU_ACCESS_WRITE表示CPU可以写入该缓冲区。对于DYNAMIC类型的缓冲区，必须设置CPU访问权限。
    
    memcpy_s(m_Vertices, sizeof(m_Vertices), vertices, sizeof(vertices));   // 将顶点数据复制到成员变量m_Vertices中，这样我们就可以在需要更新顶点数据时直接修改m_Vertices数组，然后再将其复制到GPU缓冲区中。

    // 新建顶点缓冲区
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = m_Vertices;// 使用成员变量m_Vertices的地址作为初始数据，这样在创建缓冲区时就会将这些数据复制到GPU内存中。我们之前定义了一个局部变量vertices来存储顶点数据，
                                  // 但为了后续可能需要修改顶点数据，我们将其复制到成员变量m_Vertices中，并使用m_Vertices的地址来初始化顶点缓冲区。这样我们就可以在UpdateScene函数中
                                  // 直接修改m_Vertices数组中的数据，然后再通过映射的方式更新GPU缓冲区中的数据，实现动态修改顶点信息的效果。
                                  // 在创建资源时提供初始数据，这里我们将顶点数组的地址赋值给pSysMem成员，以便在创建顶点缓冲区时将这些数据复制到GPU内存中。
   
    HR(m_pd3dDevice->CreateBuffer(&vbd, 
        &InitData, 
        m_pVertexBuffer.GetAddressOf()) // 这里要传入一个指向ComPtr<ID3D11Buffer>的地址，以便CreateBuffer函数能够将创建好的缓冲区对象的地址写入到m_pVertexBuffer中
    );                                  // 注意是双指针传入，因为CreateBuffer函数需要修改m_pVertexBuffer的值，使其指向新创建的缓冲区对象。
                                        // 这里m_pVertexBuffer是一个ComPtr对象，GetAddressOf()函数返回一个指向内部指针的地址，以满足CreateBuffer函数的参数要求。


    // ******************
    // 索引数组
    // 这是规定了三角面绘画顺序的索引数组，使用索引可以避免顶点数据的重复，节省内存和提高效率
    DWORD indices[] = {
        // 立方体
        // 正面
        0, 1, 2,
        2, 3, 0,
        // 左面
        4, 5, 1,
        1, 0, 4,
        // 顶面
        1, 5, 6,
        6, 2, 1,
        // 背面
        7, 6, 5,
        5, 4, 7,
        // 右面
        3, 2, 6,
        6, 7, 3,
        // 底面
        4, 0, 3,
        3, 7, 4,

        // 四棱锥
        9, 8, 10,
        9, 12, 8,
        9, 11, 12,
        9, 10, 11,
        12, 11, 10,
        12, 10, 8
    };
    // 设置索引缓冲区描述
    D3D11_BUFFER_DESC ibd;                      // Index Buffer Description的缩写,从底层来看，无论是vbd还是ibd，本质都是一块内存，所以它们的描述结构体是一样的，只不过我们为了代码的可读性和语义清晰，
                                                // 分别定义了vbd和ibd来描述顶点缓冲区和索引缓冲区。       
    ZeroMemory(&ibd, sizeof(ibd));
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof indices;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    // 新建索引缓冲区
    InitData.pSysMem = indices;
    HR(m_pd3dDevice->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.GetAddressOf()));
    // 输入装配阶段的索引缓冲区设置
    m_pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    // Device负责分配内存，创建资源，而上下文负责管理资源的使用和状态，控制渲染流水线。
    // IASetIndexBuffer函数将索引缓冲区绑定到输入装配阶段，告诉GPU在绘制时使用这个索引缓冲区来获取顶点索引数据。参数说明：
    // m_pIndexBuffer.Get()：获取索引缓冲区对象的指针。
    // DXGI_FORMAT_R32_UINT：指定索引数据的格式，这里是32位无符号整数。
    // 0：索引数据的起始偏移量，单位是字节，这里我们从索引缓冲区的开头开始使用。

    // ******************
    // 设置常量缓冲区描述
    //
    D3D11_BUFFER_DESC cbd;
    ZeroMemory(&cbd, sizeof(cbd));
    cbd.Usage = D3D11_USAGE_DYNAMIC;                // IMMUTABLE表示只能被GPU读取，不能被CPU修改。适用于一次创建后不再修改的数据，如静态顶点数据。
                                                    // DYNAMIC表示可以被CPU写入，但只能被GPU读取，适用于需要频繁更新的数据，如每帧更新的常量缓冲区。
    cbd.ByteWidth = sizeof(ConstantBuffer);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;    // 正是因为我们要频繁修改，所以必须给CPU赋予写入权限。
    // 新建常量缓冲区，不使用初始数据
    HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffer.GetAddressOf()));    

    // 初始化常量缓冲区的值
    // 如果你不熟悉这些矩阵，可以先忽略，待读完第四章后再回头尝试修改
    m_CBuffer.world = XMMatrixIdentity();	// 单位矩阵的转置是它本身
    m_CBuffer.view = XMMatrixTranspose(XMMatrixLookAtLH(    //LookAtLH的意思是：构建一个Left-Handed坐标系的注视摄像机，参数分别是：摄像机位置、目标点位置、上方向向量
        XMVectorSet(0.0f, 0.0f, -10.0f, 0.0f),  // 相机从-5f退到-10f，离场景更远了，能看到更多的东西
        XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
    ));
    m_CBuffer.proj = XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV2, AspectRatio(), 1.0f, 1000.0f));
    // XMMatrixPerspectiveFovLH 投影 / 镜头矩阵
    // PerspectiveFovLH的意思是：构建一个Left-Handed坐标系下的透视镜头（Field of View）镜头。
    // 参数分别是：视野角（垂直方向上的视野范围，单位是弧度，这里设置为90度，即XM_PIDIV2），        这里其实是Π/2 = 90度。
    // 宽高比（AspectRatio()函数返回屏幕的宽高比），近裁剪面距离（1.0f）和远裁剪面距离（1000.0f）。
    // 这些参数定义了摄像机的视锥体，决定了哪些物体会被渲染出来，以及它们在屏幕上的投影效果。

    // ******************
    // 给渲染管线各个阶段绑定好所需资源
    //

    // 输入装配阶段的顶点缓冲区设置
    UINT stride = sizeof(VertexPosColor);	// 跨越字节数
    UINT offset = 0;						// 起始偏移量

    m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
    // 设置图元类型，设定输入布局
    m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout.Get());
    // 将着色器绑定到渲染管线
    m_pd3dImmediateContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);         //注意只能放一个shader，多个shader要多声明几个ComPtr变量来分别存储它们的指针
    // 将更新好的常量缓冲区绑定到顶点着色器
    m_pd3dImmediateContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

    m_pd3dImmediateContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

    // ******************
    // 设置调试对象名
    //
    D3D11SetDebugObjectName(m_pVertexLayout.Get(), "VertexPosColorLayout");
    D3D11SetDebugObjectName(m_pVertexBuffer.Get(), "VertexBuffer");
    D3D11SetDebugObjectName(m_pIndexBuffer.Get(), "IndexBuffer");
    D3D11SetDebugObjectName(m_pConstantBuffer.Get(), "ConstantBuffer");
    D3D11SetDebugObjectName(m_pVertexShader.Get(), "Cube_VS");
    D3D11SetDebugObjectName(m_pPixelShader.Get(), "Cube_PS");

    return true;
}
