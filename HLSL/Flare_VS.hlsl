#include "Basic.hlsli"

// Flare 模式顶点着色器常量缓冲区
cbuffer FlareCB : register(b2)
{
    matrix g_TexRotation;  // 纹理坐标旋转矩阵（4x4）
}

// 顶点着色器：标准 MVP 变换 + 传递纹理坐标
VertexPosHTex VS(VertexPosNormalTex vIn)
{
    VertexPosHTex vOut;
    matrix viewProj = mul(g_View, g_Proj);
    float4 posW = mul(float4(vIn.posL, 1.0f), g_World);
    vOut.posH = mul(posW, viewProj);
    vOut.tex = vIn.tex;
    return vOut;
}
