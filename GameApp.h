#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "LightHelper.h"    // 光照结构体定义（方向光、点光、聚光灯、材质）
#include "Geometry.h"       // 几何体生成工具（创建立方体、球体、圆柱体、圆锥体等）

//===========================================================
// GameApp 类 - 游戏主程序
// 
// 继承自 D3DApp，管理 Direct3D 11 的渲染流程
// 这个版本专注于光照渲染，展示了三种光源类型：
//   1. 方向光（Directional Light）- 模拟太阳等远距离光源
//   2. 点光源（Point Light）     - 模拟灯泡等有位置的光源
//   3. 聚光灯（Spot Light）      - 模拟手电筒等锥形光源
//===========================================================
class GameApp : public D3DApp
{
public:

    //------------------------------------------------------
    // VSConstantBuffer - 顶点着色器常量缓冲区（CPU端）
    // 对应 HLSL 中的 VSConstantBuffer（寄存器 b0）
    // 负责将世界矩阵、观察矩阵、投影矩阵等传入 GPU
    // 这些矩阵决定了"物体应该画在屏幕的什么位置"
    //------------------------------------------------------
    struct VSConstantBuffer
    {
        DirectX::XMMATRIX world;              // 世界矩阵：将模型从局部坐标系变换到世界坐标系
        DirectX::XMMATRIX view;               // 观察矩阵：从世界变到以摄像机为原点的视图坐标系
        DirectX::XMMATRIX proj;               // 投影矩阵：将3D视锥投影到2D屏幕（透视效果）
        DirectX::XMMATRIX worldInvTranspose;  // 世界矩阵的逆转置矩阵：用于正确变换法向量
                                               // （当模型有非均匀缩放时，法向量必须用此矩阵）
    };

    //------------------------------------------------------
    // PSConstantBuffer - 像素着色器常量缓冲区（CPU端）
    // 对应 HLSL 中的 PSConstantBuffer（寄存器 b1）
    // 包含所有的光照参数，每帧更新到 GPU
    //------------------------------------------------------
    struct PSConstantBuffer
    {
        DirectionalLight dirLight;    // 方向光（平行光）参数
        PointLight pointLight;        // 点光源参数
        SpotLight spotLight;          // 聚光灯参数
        Material material;            // 物体材质属性
        DirectX::XMFLOAT4 eyePos;     // 摄像机位置（xyz），用于计算镜面高光
    };



public:
    GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight);
    ~GameApp();

    bool Init();            // 初始化：调用 InitEffect() 和 InitResource()
    void OnResize();        // 窗口大小改变时的处理
    void UpdateScene(float dt);  // 每帧更新逻辑（物体旋转 + ImGui 界面）
    void DrawScene();       // 每帧绘制场景


private:
    bool InitEffect();      // 创建/加载顶点着色器和像素着色器
    bool InitResource();    // 初始化网格、常量缓冲区、光照参数等
    bool ResetMesh(const Geometry::MeshData<VertexPosNormalColor>& meshData);  // 切换显示不同类型的网格


private:
    // ---- 顶点/索引缓冲区 ----
    ComPtr<ID3D11InputLayout> m_pVertexLayout;	    // 顶点输入布局（描述顶点数据的格式）
    ComPtr<ID3D11Buffer> m_pVertexBuffer;			// 顶点缓冲区（存储顶点数据）
    ComPtr<ID3D11Buffer> m_pIndexBuffer;			// 索引缓冲区（指定绘制顶点的顺序）
    ComPtr<ID3D11Buffer> m_pConstantBuffers[2];	    // 常量缓冲区数组：[0] VS, [1] PS
    UINT m_IndexCount;							    // 当前模型的索引数量（决定画多少个三角形）

    // ---- 着色器 ----
    ComPtr<ID3D11VertexShader> m_pVertexShader;	    // 顶点着色器（处理顶点变换）
    ComPtr<ID3D11PixelShader> m_pPixelShader;		// 像素着色器（计算像素颜色）
    ComPtr<ID3D11PixelShader> m_pWireframePS;		// 线框像素着色器（输出固定白色，用于叠加三角形边界）
    
    // ---- 常量缓冲区（CPU端副本） ----
    VSConstantBuffer m_VSConstantBuffer;			// CPU端的VS常量数据，每帧修改后上传到GPU
    PSConstantBuffer m_PSConstantBuffer;			// CPU端的PS常量数据，每帧修改后上传到GPU

    // ---- 默认光照预设 ----
    DirectionalLight m_DirLight;					// 预设的方向光（默认开启）
    PointLight m_PointLight;						// 预设的点光源
    SpotLight m_SpotLight;						    // 预设的聚光灯

    // ---- 渲染状态 ----
    ComPtr<ID3D11RasterizerState> m_pRS[6];   // 6种组合: [FillMode*3 + CullMode]
    ComPtr<ID3D11DepthStencilState> m_pDSEqual;  // 深度比较 LESS_EQUAL（用于第二遍wireframe叠加）
    int m_FillMode;                            // 0=Solid, 1=Wireframe
    int m_CullMode;                            // 0=None, 1=Back, 2=Front
    bool m_ShowTriangleEdges;						// 是否叠加三角形边界线（独立checkbox控制）
    
};


#endif
