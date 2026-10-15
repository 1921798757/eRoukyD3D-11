
#ifndef RENDERING_HLSL
#define RENDERING_HLSL

#include "FullScreenTriangle.hlsl"
#include "ConstantBuffers.hlsl"

//--------------------------------------------------------------------------------------
// 工具函数 
//--------------------------------------------------------------------------------------
float linstep(float min_value, float max_value, float value)
{
    return saturate((value - min_value) / (max_value - min_value));
}

//--------------------------------------------------------------------------------------
// 几何阶段 
//--------------------------------------------------------------------------------------
// t0：漫反射贴图；s0：与之配套的采样器
Texture2D g_DiffuseMap : register(t0);
SamplerState g_Sam : register(s0);

// 顶点着色器输入：来自顶点缓冲，全部在局部空间
struct VertexPosNormalTex
{
    float3 posL : POSITION;    // 局部空间位置
    float3 normalL : NORMAL;   // 局部空间法线
    float2 texCoord : TEXCOORD; // 纹理坐标
};

// 顶点着色器输出 / 像素着色器输入
struct VertexPosHVNormalVTex
{
    float4 posH : SV_POSITION;  // 齐次裁剪空间位置，供光栅化与深度测试使用
    float3 posV : POSITION;     // 视图空间（相机空间）位置，光照计算用
    float3 normalV : NORMAL;    // 视图空间法线，光照计算用
    float2 texCoord : TEXCOORD; // 纹理坐标
};

VertexPosHVNormalVTex GeometryVS(VertexPosNormalTex input)
{
    VertexPosHVNormalVTex output;

    // 局部空间 -> 齐次裁剪空间（位置补 w=1，保留齐次坐标）
    output.posH = mul(float4(input.posL, 1.0f), g_WorldViewProj);
    // 局部空间 -> 视图空间（位置补 w=1，取 xyz）
    output.posV = mul(float4(input.posL, 1.0f), g_WorldView).xyz;
    // 局部空间法线 -> 视图空间（法线是方向，补 w=0，避免被矩阵的平移部分影响）
    output.normalV = mul(float4(input.normalL, 0.0f), g_WorldInvTransposeView).xyz;
    output.texCoord = input.texCoord;
    
    return output;
}

float3 ComputeFaceNormal(float3 pos)
{
    return cross(ddx_coarse(pos), ddy_coarse(pos));
}

struct SurfaceData
{
    float3 posV;
    float3 posV_DX;
    float3 posV_DY;
    float3 normalV;
    float4 albedo;
    float specularAmount;
    float specularPower;
};

SurfaceData ComputeSurfaceDataFromGeometry(VertexPosHVNormalVTex input)
{
    SurfaceData surface;
    surface.posV = input.posV;
    
    // 右/下相邻像素与当前像素的位置差
    surface.posV_DX = ddx_coarse(surface.posV);
    surface.posV_DY = ddy_coarse(surface.posV);
    
    // 该表面法线可用于替代提供的法线
    float3 faceNormal = ComputeFaceNormal(input.posV);
    surface.normalV = normalize(g_FaceNormals ? faceNormal : input.normalV);
    
    surface.albedo = g_DiffuseMap.Sample(g_Sam, input.texCoord);
    surface.albedo.rgb = g_LightingOnly ? float3(1.0f, 1.0f, 1.0f) : surface.albedo.rgb;
    
    // 我们没有艺术资产相关的值来设置下面这些，现在就设置成看起来比较合理的值
    surface.specularAmount = 0.9f;
    surface.specularPower = 25.0f;
    
    return surface;
}

//--------------------------------------------------------------------------------------
// 光照阶段 
//--------------------------------------------------------------------------------------
struct PointLight
{
    float3 posV;
    float attenuationBegin;
    float3 color;
    float attenuationEnd;
};

// 大量的动态点光源
StructuredBuffer<PointLight> g_Light : register(t5);

// 这里分成diffuse/specular项方便延迟光照使用
void AccumulatePhong(float3 normal, float3 lightDir, float3 viewDir, float3 lightContrib, float specularPower,
                     inout float3 litDiffuse, inout float3 litSpecular)
{
    float NdotL = dot(normal, lightDir);
    [flatten]
    if (NdotL > 0.0f)
    {
        float3 r = reflect(lightDir, normal);
        float RdotV = max(0.0f, dot(r, viewDir));
        float specular = pow(RdotV, specularPower);
        
        litDiffuse += lightContrib * NdotL;
        litSpecular += lightContrib * specular;
    }
}

void AccumulateDiffuseSpecular(SurfaceData surface, PointLight light,
                               inout float3 litDiffuse, inout float3 litSpecular)
{
    float3 dirToLight = light.posV - surface.posV;
    float distToLight = length(dirToLight);
    
    [branch]
    if (distToLight < light.attenuationEnd)
    {
        float attenuation = linstep(light.attenuationEnd, light.attenuationBegin, distToLight);
        dirToLight *= rcp(distToLight);
        AccumulatePhong(surface.normalV, dirToLight, normalize(surface.posV),
            attenuation * light.color, surface.specularPower, litDiffuse, litSpecular);
    }
    
}

void AccumulateColor(SurfaceData surface, PointLight light,
                     inout float3 litColor)
{
    float3 dirToLight = light.posV - surface.posV;
    float distToLight = length(dirToLight);
    
    [branch]
    if (distToLight < light.attenuationEnd)
    {
        float attenuation = linstep(light.attenuationEnd, light.attenuationBegin, distToLight);
        dirToLight *= rcp(distToLight);
        
        float3 litDiffuse = float3(0.0f, 0.0f, 0.0f);
        float3 litSpecular = float3(0.0f, 0.0f, 0.0f);
        AccumulatePhong(surface.normalV, dirToLight, normalize(surface.posV),
            attenuation * light.color, surface.specularPower, litDiffuse, litSpecular);
        litColor += surface.albedo.rgb * (litDiffuse + surface.specularAmount * litSpecular);
    }
}

#endif // RENDERING_HLSL
