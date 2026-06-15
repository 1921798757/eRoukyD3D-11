#include "Light.hlsli"   

//---------------------------------------------------------
// 像素着色器(Pixel Shader) - 光照版本
// 
// 作用：计算每个像素的最终颜色
// 应用了三种光源的叠加效果：方向光 + 点光 + 聚光灯光
// 每种光源都包含环境光(ambient)、漫反射光(diffuse)、镜面光(specular)三个分量
//---------------------------------------------------------
float4 PS(VertexOut pIn) : SV_Target
{
    //-----------------------------------------------------
    // 步骤1：准备光照计算所需的向量
    //-----------------------------------------------------
    
    // 标准化法向量：经过顶点着色器插值后的法向量长度可能会变化，需要重新归一化
    pIn.normalW = normalize(pIn.normalW);

    // 计算从顶点指向眼睛（摄像机）的向量
    // g_EyePosW 是摄像机在世界空间的位置，pIn.posW 是当前像素对应的世界空间位置
    float3 toEyeW = normalize(g_EyePosW - pIn.posW);

    //-----------------------------------------------------
    // 步骤2：分别计算三种光源的贡献，然后叠加
    //-----------------------------------------------------
    
    // 初始化各种颜色分量为0
    float4 ambient, diffuse, spec;   // 最终累计的环境/漫反射/镜面光分量
    float4 A, D, S;                  // 临时存储单个光源的计算结果
    ambient = diffuse = spec = A = D = S = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // (1) 计算方向光（平行光）的贡献
    ComputeDirectionalLight(g_Material, g_DirLight, pIn.normalW, toEyeW, A, D, S);
    ambient += A;   // 累加环境光
    diffuse += D;   // 累加漫反射光
    spec += S;      // 累加镜面光（高光）

    // (2) 计算点光源的贡献
    ComputePointLight(g_Material, g_PointLight, pIn.posW, pIn.normalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;

    // (3) 计算聚光灯的贡献
    ComputeSpotLight(g_Material, g_SpotLight, pIn.posW, pIn.normalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;

    //-----------------------------------------------------
    // 步骤3：合成最终颜色
    // 使用 Phong 光照模型：
    //   最终颜色 = 顶点颜色 × (环境光 + 漫反射光) + 镜面光（高光）
    // 
    // 为什么镜面光是直接加而不是乘？
    //   环境光和漫反射光取决于物体本身的颜色，所以要乘顶点颜色
    //   镜面光（高光）是光源颜色在光滑表面的反射，不依赖于物体本身的颜色
    //-----------------------------------------------------
    float4 litColor = pIn.color * (ambient + diffuse) + spec;
    
    // 设置 alpha 通道（透明度）：使用材质的漫反射 alpha × 顶点颜色 alpha
    litColor.a = g_Material.diffuse.a * pIn.color.a;
    
    return litColor;
}
