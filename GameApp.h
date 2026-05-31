#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"

// ------------------------------
// 继承自D3DAPP
// ------------------------------
// 
class GameApp : public D3DApp
{
public:
    struct VertexPosColor
    {
        DirectX::XMFLOAT3 pos;      // 12Bytes
        DirectX::XMFLOAT4 color;    // 16Bytes
        static const D3D11_INPUT_ELEMENT_DESC inputLayout[2];   // 虽然我们C++层定义了一个VertexPosColor结构体来描述顶点数据，但GPU并不知道这个结构体是什么样子的，
                                                                // 我们需要告诉GPU这个结构体的内存布局，这就是inputLayout的作用。
                                                                // inputLayout是一个数组，每个元素描述了一个顶点属性（如位置、颜色等）的格式和在内存中的偏移。
                                                                // 这样GPU就能正确地从顶点缓冲区中读取数据，并将其传递给顶点着色器。
    };

    struct ConstantBuffer
    {
        DirectX::XMMATRIX world;
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX proj;
    };

public:
    GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight);
    ~GameApp();

    bool Init();
    void OnResize();
    void UpdateScene(float dt);
    void DrawScene();


private:
    bool InitEffect();
    bool InitResource();



private:                                            // 这里m_是member首字母，类成员变量的前缀，为了规范
    ComPtr<ID3D11InputLayout> m_pVertexLayout;      // 顶点输入布局
    ComPtr<ID3D11Buffer> m_pVertexBuffer;           // 顶点缓冲区
    ComPtr<ID3D11Buffer> m_pIndexBuffer;            // 索引缓冲区
    ComPtr<ID3D11Buffer> m_pConstantBuffer;         // 常量缓冲区

    ComPtr<ID3D11VertexShader> m_pVertexShader;     // 顶点着色器
    ComPtr<ID3D11PixelShader> m_pPixelShader;       // 像素着色器
    ConstantBuffer m_CBuffer;                       // 用于修改GPU常量缓冲区的变量

    DirectX::XMFLOAT4X4 m_BaseRotation;             // 基础旋转矩阵，表示立方体的初始旋转状态，
};


#endif