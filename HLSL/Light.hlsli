

#include "LightHelper.hlsli"   // 引入光照计算辅助函数和结构体定义

//---------------------------------------------------------
// 顶点着色器(Vertex Shader)常量缓冲区 - 绑定到寄存器 b0
// 对应 C++ 端 GameApp.h 中的 VSConstantBuffer
// 这里的数据在每帧更新（物体旋转时，世界矩阵会变化）
//---------------------------------------------------------
cbuffer VSConstantBuffer : register(b0)
{
    matrix g_World;             // 世界矩阵：将顶点从局部坐标系变换到世界坐标系
    matrix g_View;              // 观察矩阵：从世界坐标系变换到以摄像机为原点的视角坐标系  
    matrix g_Proj;              // 投影矩阵：将视锥体映射到归一化设备坐标系(NDC)
    matrix g_WorldInvTranspose; // 世界矩阵的逆转置矩阵：用于正确变换法向量
                                // （当世界矩阵包含非均匀缩放时，法向量必须用这个矩阵变换才能保持垂直）
}

//---------------------------------------------------------
// 像素着色器(Pixel Shader)常量缓冲区 - 绑定到寄存器 b1
// 对应 C++ 端 GameApp.h 中的 PSConstantBuffer
// 包含光照参数和相机位置等信息
//---------------------------------------------------------
cbuffer PSConstantBuffer : register(b1)
{
    DirectionalLight g_DirLight;    // 方向光（平行光）：如太阳光，光线方向恒定
    PointLight g_PointLight;        // 点光源：从一点向四周均匀发射光线，如灯泡
    SpotLight g_SpotLight;          // 聚光灯光源：锥形范围的光，如手电筒
    Material g_Material;            // 物体表面材质属性（环境光反射率、漫反射率、镜面反射率）
    float3 g_EyePosW;               // 观察者（摄像机）在世界空间中的位置，用于计算镜面高光
    float g_Pad;                    // 填充字段：确保结构体大小为16的倍数（HLSL的cbuffer对齐要求），只是填充
}


//---------------------------------------------------------
// 顶点输入结构：描述每个顶点中包含的数据
// 对应 C++ 端 Vertex.h 中的 VertexPosNormalColor
//---------------------------------------------------------
struct VertexIn
{
    float3 posL    : POSITION;    // 局部坐标系下的顶点位置
    float3 normalL : NORMAL;      // 局部坐标系下的顶点法向量
    float4 color   : COLOR;       // 顶点颜色
};

//---------------------------------------------------------
// 顶点着色器输出 / 像素着色器输入结构
// 顶点着色器处理后会传递这些数据到像素着色器
//---------------------------------------------------------
struct VertexOut
{
    float4 posH    : SV_POSITION; // 齐次裁剪空间下的顶点位置（屏幕坐标），SV_POSITION语义表示这是系统用的位置
    float3 posW    : POSITION;    // 顶点在世界空间中的位置（用于光照计算）
    float3 normalW : NORMAL;      // 法向量在世界空间中的方向（用于光照计算）
    float4 color   : COLOR;       // 经过传递的顶点颜色
};


