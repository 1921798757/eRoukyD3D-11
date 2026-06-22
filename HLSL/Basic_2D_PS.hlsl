#include "LightHelper.hlsli"

// 火焰纹理数组：120帧动画存储为Texture2DArray
Texture2DArray g_FireTex : register(t0);
SamplerState g_SamLinear : register(s0);

cbuffer PSConstantBuffer : register(b1)
{
    DirectionalLight g_DirLight[10];
    PointLight g_PointLight[10];
    SpotLight g_SpotLight[10];
    Material g_Material;
    int g_NumDirLight;
    int g_NumPointLight;
    int g_NumSpotLight;
    int g_FireFrame;

    float3 g_EyePosW;
    float g_Pad2;
}

struct VertexPosHTex
{
    float4 posH : SV_POSITION;
    float2 tex : TEXCOORD;
};

// 像素着色器(2D) - 使用Texture2DArray，通过g_FireFrame选择帧
float4 PS(VertexPosHTex pIn) : SV_Target
{
    return g_FireTex.Sample(g_SamLinear, float3(pIn.tex, g_FireFrame));
}