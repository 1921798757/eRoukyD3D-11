cbuffer ConstantBuffer : register(b0)
{
    matrix g_World;
    matrix g_View;
    matrix g_Proj;
}

struct VertexIn
{
    float3 posL : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

struct VertexOut
{
    float4 posH : SV_POSITION;
    float2 tex : TEXCOORD;
};

// 纹理和采样器
Texture2D g_FaceTexture : register(t0);
SamplerState g_SamLinear : register(s0);