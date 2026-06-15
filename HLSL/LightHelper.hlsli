
//=========================================================
// LightHelper.hlsli - 光照计算辅助函数库（GPU端）
// 
// 这个文件定义了CPU端 LightHelper.h 中对应的光照结构体（HLSL版本）
// 以及三种光源类型的颜色计算公式
// 注意：C++端的 LightHelper.h 结构体定义必须与这里的布局一致，
// 因为 memcpy 直接把CPU数据拷贝到GPU常量缓冲区
//=========================================================

//---------------------------------------------------------
// 方向光（平行光）结构体
// 特点：所有光线来自同一方向，强度不随距离衰减（如太阳光）
// 打包规则：每个成员对齐到16字节边界
//---------------------------------------------------------
struct DirectionalLight
{
    float4 ambient;      // 环境光颜色和强度（rgba）
    float4 diffuse;      // 漫反射光颜色和强度（rgba）
    float4 specular;     // 镜面高光颜色和强度（rgba）
    float3 direction;    // 光线方向向量（从光源指向物体）
                         // 注意：实际计算时取反才是光照方向
    float pad;           // 填充：确保结构体大小是16的倍数
};

//---------------------------------------------------------
// 点光源（Point Light）结构体
// 特点：从一个点向四面八方发射光线，强度随距离衰减（如灯泡）
// 打包规则：为了满足HLSL的16字节对齐要求，将float3和float打包成一个4D向量
//---------------------------------------------------------
struct PointLight
{
    float4 ambient;      // 环境光颜色和强度
    float4 diffuse;      // 漫反射光颜色和强度
    float4 specular;     // 镜面高光颜色和强度

    float3 position;     // 光源在世界空间中的位置
    float range;         // 光源的有效照射范围（超出此范围不再计算光照）

    float3 att;          // 衰减系数（衰减公式：1/(att.x + att.y*d + att.z*d²)）
                         // att.x = 常数项，att.y = 一次项，att.z = 二次项
    float pad;           // 填充字节，满足16字节对齐
};

//---------------------------------------------------------
// 聚光灯（Spot Light）结构体
// 特点：锥形范围的光，既有方向又有位置（如手电筒、舞台聚光灯）
//        在点光源基础上增加了方向限制，只有锥体内的物体才被照亮
//---------------------------------------------------------
struct SpotLight
{
    float4 ambient;      // 环境光颜色和强度
    float4 diffuse;      // 漫反射光颜色和强度
    float4 specular;     // 镜面高光颜色和强度

    float3 position;     // 光源在世界空间中的位置
    float range;         // 光源的有效照射范围

    float3 direction;    // 聚光灯的朝向（锥体中心轴的方向）
    float Spot;          // 汇聚指数：控制锥角大小和边缘锐利度
                         // 值越大，光锥越窄且边缘越锐利

    float3 att;          // 衰减系数：1/(att.x + att.y*d + att.z*d²)
    float pad;           // 填充字节，满足16字节对齐
};

//---------------------------------------------------------
// 物体表面材质（Material）结构体
// 描述物体表面的光学属性
// 注意：材质属性与光照强度进行逐分量乘法运算
//---------------------------------------------------------
struct Material
{
    float4 ambient;      // 环境光反射率：物体对环境光的反射系数
    float4 diffuse;      // 漫反射率：物体对漫反射光的反射系数（= 物体本身的颜色）
    float4 specular;     // 镜面反射率：xyz=高光颜色，w=镜面反射强度指数(Specular Power)
                         // SpecPower越大，高光点越小越亮（越像金属）
    float4 reflect;      // 反射属性：用于后续的反射效果
};



//===========================================================
// 以下是三种光源的颜色计算函数
// 基于经典的 Phong 光照模型：
//   最终颜色 = 环境光 + 漫反射光 + 镜面高光
//===========================================================

//---------------------------------------------------------
// 方向光（平行光）颜色计算
// 参数说明：
//   mat: 材质属性
//   L:   光源属性
//   normal: 表面法向量（已归一化）
//   toEye:  从表面指向观察者的向量（已归一化）
//   out ambient/diffuse/spec: 输出的环境/漫反射/镜面光颜色分量
//---------------------------------------------------------
void ComputeDirectionalLight(Material mat, DirectionalLight L,
    float3 normal, float3 toEye,
    out float4 ambient,
    out float4 diffuse,
    out float4 spec)
{
    // 初始化输出为黑色（无光）
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // 光向量取反：HLSL中direction是"从光源指向物体"的方向
    // 而dot(lightVec, normal)需要"从物体指向光源"的方向
    float3 lightVec = -L.direction;

    // 环境光计算：不受光照方向影响，只是简单地相乘
    // ambient = 材质的环境光反射率 × 光源的环境光强度
    ambient = mat.ambient * L.ambient;

    // 漫反射和镜面光计算：需要知道光照方向与法线的夹角
    // diffuseFactor = cosθ（θ为入射光与法线的夹角）
    // 兰伯特余弦定律：当光垂直照射时最亮，斜射时变暗
    float diffuseFactor = dot(lightVec, normal);

    // [flatten] 提示编译器：不要生成条件分支指令，而是用条件移动指令
    // 这样可以避免GPU中的分支发散（warp divergence）问题
    // [flatten] 这个标志让生成的底层机器码不再包含条件跳转，GPU会让所有的像素把if内部的所有代码全执行一遍，然后根据条件结果选择最终的输出
    [flatten]
    if (diffuseFactor > 0.0f)
    {
        // 计算反射向量 v：用于镜面高光计算
        // reflect(i, n) = i - 2 * dot(i, n) * n，其中i是入射光向量
        float3 v = reflect(-lightVec, normal);
        
        // 镜面因子 = (反射光与视线方向的点积)^镜面强度指数
        // pow(x, n) 中的 n(mat.specular.w) 越大，高光越集中、越小
        // 例如：塑料的镜面指数~10，金属的镜面指数~100+
        float specFactor = pow(max(dot(v, toEye), 0.0f), mat.specular.w);

        // 漫反射颜色 = cosθ × 材质漫反射色 × 光的漫反射色
        diffuse = diffuseFactor * mat.diffuse * L.diffuse;
        // 镜面高光颜色 = 镜面因子 × 材质镜面色 × 光的镜面色
        spec = specFactor * mat.specular * L.specular;
    }
}

//---------------------------------------------------------
// 点光源颜色计算
// 与方向光的区别：位置固定，距离不同光照强度不同（有衰减）
// 参数比方向光多一个 pos：像素在世界空间的位置
//---------------------------------------------------------
void ComputePointLight(Material mat, PointLight L, float3 pos, float3 normal, float3 toEye,
    out float4 ambient, out float4 diffuse, out float4 spec)
{
    // 初始化输出为黑色（无光）
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // 从像素位置指向光源位置的向量
    float3 lightVec = L.position - pos;

    // 计算像素到光源的距离
    float d = length(lightVec);

    // 灯光范围测试：如果超出光源照射范围，直接返回（不计算光照）
    if (d > L.range)
        return;

    // 标准化光向量（长度为1，只保留方向信息）
    lightVec /= d;

    // 环境光计算（与方向光相同）
    ambient = mat.ambient * L.ambient;

    // 漫反射和镜面计算（与方向光相同）
    float diffuseFactor = dot(lightVec, normal);

    [flatten]
    if (diffuseFactor > 0.0f)
    {
        float3 v = reflect(-lightVec, normal);
        float specFactor = pow(max(dot(v, toEye), 0.0f), mat.specular.w);

        diffuse = diffuseFactor * mat.diffuse * L.diffuse;
        spec = specFactor * mat.specular * L.specular;
    }

    // 光的衰减：随着距离增加，光照强度减弱
    // 衰减公式：1 / (A0 + A1*d + A2*d²)
    // A0: 常数衰减，A1: 线性衰减，A2: 二次衰减
    // 二次衰减模拟了真实光的物理行为（平方反比定律）
    float att = 1.0f / dot(L.att, float3(1.0f, d, d * d));

    // 将衰减系数应用到漫反射和镜面光
    // 注意：环境光不受距离影响
    diffuse *= att;
    spec *= att;
}

//---------------------------------------------------------
// 聚光灯颜色计算
// 特点：在点光源的基础上增加了方向限制（圆锥形照射范围）
// 只有位于圆锥内的像素才被照亮，且有边缘衰减
//---------------------------------------------------------
void ComputeSpotLight(Material mat, SpotLight L, float3 pos, float3 normal, float3 toEye,
    out float4 ambient, out float4 diffuse, out float4 spec)
{
    // 初始化输出为黑色（无光）
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // 从像素位置指向光源位置的向量
    float3 lightVec = L.position - pos;

    // 计算像素到光源的距离
    float d = length(lightVec);

    // 范围测试：超出照射范围直接返回
    if (d > L.range)
        return;

    // 标准化光向量
    lightVec /= d;

    // 计算环境光部分
    ambient = mat.ambient * L.ambient;


    // 计算漫反射光和镜面反射光部分（与方向光相同）
    float diffuseFactor = dot(lightVec, normal);

    [flatten]
    if (diffuseFactor > 0.0f)
    {
        float3 v = reflect(-lightVec, normal);
        float specFactor = pow(max(dot(v, toEye), 0.0f), mat.specular.w);

        diffuse = diffuseFactor * mat.diffuse * L.diffuse;
        spec = specFactor * mat.specular * L.specular;
    }

    // 计算汇聚因子(spot)和衰减系数(att)
    // 汇聚因子：控制光锥形状，dot(-lightVec, L.direction) 是
    // "从光源指向像素的方向"与"光源朝向"的夹角余弦
    // pow(..., L.Spot)的指数越大，光锥越窄
    float spot = pow(max(dot(-lightVec, L.direction), 0.0f), L.Spot);
    
    // 衰减 = spot / (A0 + A1*d + A2*d²)
    // 分子多了spot：远离光锥中心轴时，光照强度也减弱
    float att = spot / dot(L.att, float3(1.0f, d, d * d));

    // 环境光也受汇聚因子影响（在光锥外的物体不应受聚光灯光的环境光影响）
    ambient *= spot;
    diffuse *= att;
    spec *= att;
}
