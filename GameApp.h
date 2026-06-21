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
        DirectionalLight dirLight[10];
        PointLight pointLight[10];
        SpotLight spotLight[10];
        Material material;
        int numDirLight;
        int numPointLight;
        int numSpotLight;
        float pad;		// 打包保证16字节对齐
        DirectX::XMFLOAT4 eyePos;
    };

    enum class ShowMode { WoodCrate, FireAnim };

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
    template<class VertexType>
    bool ResetMesh(const Geometry::MeshData<VertexType>& meshData);


private:
    ComPtr<ID3D11InputLayout> m_pVertexLayout2D;				// 用于2D的顶点输入布局
    ComPtr<ID3D11InputLayout> m_pVertexLayout3D;				// 用于3D的顶点输入布局
    ComPtr<ID3D11Buffer> m_pVertexBuffer;						// 顶点缓冲区
    ComPtr<ID3D11Buffer> m_pIndexBuffer;						// 索引缓冲区
    ComPtr<ID3D11Buffer> m_pConstantBuffers[2];				    // 常量缓冲区
    UINT m_IndexCount;										    // 绘制物体的索引数组大小
    int m_CurrFrame;											// 当前火焰动画播放到第几帧
    ShowMode m_CurrMode;										// 当前显示的模式

    ComPtr<ID3D11ShaderResourceView> m_pWoodCrate;			    // 木盒纹理
    std::vector<ComPtr<ID3D11ShaderResourceView>> m_pFireAnims; // 火焰纹理集
    ComPtr<ID3D11SamplerState> m_pSamplerState;				    // 采样器状态

    ComPtr<ID3D11VertexShader> m_pVertexShader3D;				// 用于3D的顶点着色器
    ComPtr<ID3D11PixelShader> m_pPixelShader3D;				    // 用于3D的像素着色器
    ComPtr<ID3D11VertexShader> m_pVertexShader2D;				// 用于2D的顶点着色器
    ComPtr<ID3D11PixelShader> m_pPixelShader2D;				    // 用于2D的像素着色器

    ComPtr<ID3D11ShaderResourceView> m_pFaceSRV[6];             // 立方体贴图的6个面纹理资源视图

    VSConstantBuffer m_VSConstantBuffer;						// 用于修改用于VS的GPU常量缓冲区的变量
    PSConstantBuffer m_PSConstantBuffer;						// 用于修改用于PS的GPU常量缓冲区的变量
};


#endif
