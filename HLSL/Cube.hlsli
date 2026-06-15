
cbuffer ConstantBuffer : register(b0)
{
    matrix g_World; // matrix可以用float4x4替代。不加row_major的情况下，矩阵默认为列主矩阵，
    matrix g_View;  // 可以在前面添加row_major表示行主矩阵
    matrix g_Proj;  // 该教程往后将使用默认的列主矩阵，但需要在C++代码端预先将矩阵进行转置。
    vector g_Color;
    uint g_UseCustomColor;
}


struct VertexIn
{
    float3 posL : POSITION;         // L指的是Local 局部空间
    float4 color : COLOR;
};

struct VertexOut
{
    float4 posH : SV_POSITION;      // H指的是Homogeneous Clip 齐次裁剪空间
    float4 color : COLOR;           // MVP之后就是齐次裁剪空间，NDC是在齐次裁剪空间基础上进行透视除法得到的标准化设备坐标，
                                    // 屏幕空间是在NDC基础上进行视口变换得到的。
};
