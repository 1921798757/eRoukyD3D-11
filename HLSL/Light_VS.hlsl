#include "Light.hlsli"   // 引入共享的常量缓冲区、结构体等定义

//---------------------------------------------------------
// 顶点着色器(Vertex Shader) - 光照版本
// 
// 作用：
//   1. 将顶点从局部坐标系变换到齐次裁剪空间（以便GPU渲染到屏幕）
//   2. 将顶点位置和法向量变换到世界空间（以便像素着色器进行光照计算）
//---------------------------------------------------------
VertexOut VS(VertexIn vIn)
{
    VertexOut vOut;
    
    // 将观察矩阵和投影矩阵相乘，得到 ViewProj 矩阵
    // 这样可以一次矩阵乘法完成从世界到裁剪空间的变换
    matrix viewProj = mul(g_View, g_Proj);
    
    // 将顶点位置从局部坐标系变换到世界坐标系
    // float4(vIn.posL, 1.0f) 中的 1.0 表示这是一个位置向量（而非方向向量）
    float4 posW = mul(float4(vIn.posL, 1.0f), g_World);

    // 继续变换到齐次裁剪空间（屏幕坐标）
    vOut.posH = mul(posW, viewProj);
    
    // 保存世界空间中的位置，供像素着色器做光照计算使用
    vOut.posW = posW.xyz;
    
    // 将法向量从局部坐标系变换到世界坐标系
    // 注意：使用 g_WorldInvTranspose（世界矩阵的逆转置）而不是 g_World
    // 原因：当有非均匀缩放时，法向量不能直接用世界矩阵变换，（Model 矩阵的左上角3x3的逆转置矩阵）
    // (float3x3) 是类型转换，忽略平移部分只取旋转和缩放
    vOut.normalW = mul(vIn.normalL, (float3x3) g_WorldInvTranspose);
    
    // 传递顶点颜色（alpha通道默认为1.0，即不透明）
    vOut.color = vIn.color;
    
    return vOut;
}
