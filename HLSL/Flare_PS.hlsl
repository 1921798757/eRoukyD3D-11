#include "Basic.hlsli"

// Flare 模式顶点着色器常量缓冲区（与 VS 中一致）
cbuffer FlareCB : register(b2)
{
    matrix g_TexRotation;  // 纹理坐标旋转矩阵（4x4）
}

// 第二个纹理（flarealpha.dds）
Texture2D g_TexAlpha : register(t1);

// 像素着色器：对纹理坐标施加旋转矩阵，然后分量乘法混合两个纹理
float4 PS(VertexPosHTex pIn) : SV_Target
{
    // 将纹理坐标从 [0,1] 平移到 [-0.5, 0.5]，应用旋转矩阵，再平移回 [0,1]
    float2 texCenter = float2(0.5f, 0.5f);
    float2 centeredTex = pIn.tex - texCenter;

    // 用 4x4 矩阵变换纹理坐标（只使用 xy 分量，zw 用于齐次坐标）
    float4 texVec = mul(float4(centeredTex, 0.0f, 1.0f), g_TexRotation);
    float2 rotatedTex = texVec.xy + texCenter;

    // 使用 BORDER_COLOR 采样器采样两个纹理
    float4 flareColor = g_Tex.Sample(g_SamLinear, rotatedTex);
    float4 alphaColor = g_TexAlpha.Sample(g_SamLinear, rotatedTex);

    // 分量乘法：flare 纹理 * alpha 遮罩
    return flareColor * alphaColor;
}
