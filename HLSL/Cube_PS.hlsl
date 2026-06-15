#include "Cube.hlsli"



// 像素着色器
float4 PS(VertexOut pIn) : SV_Target
{
    return g_UseCustomColor ? g_Color : pIn.color;  // 若开启了自定义颜色，那么使用自定义颜色(g_Color)，否则使用顶点颜色(pIn.color)

    // VertexOut是Cube.hlsli中定义的结构体，包含了顶点着色器输出的位置信息和颜色信息。
    // SV_Target是一个系统值语义，表示这是一个像素着色器的输出颜色。

}
