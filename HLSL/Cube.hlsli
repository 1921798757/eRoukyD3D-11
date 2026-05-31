
cbuffer ConstantBuffer : register(b0)   // cbuffer是HLSL中的常量缓冲区，用于存储在着色器中使用的常量数据。这里的“常量”不是指数据永远不能变，
                                        // 而是指在一次绘制调用（Draw Call）期间，这个数据对于所有顶点/像素来说是固定不变的。
                                        // register(b0)表示将该常量缓冲区绑定到寄存器b0上，以便在渲染过程中访问这些数据。b指的是Buffer，
                                        // 类似的还有t（Texture）和s（Sampler）。
{
    matrix g_World; // matrix可以用float4x4替代。不加row_major的情况下，矩阵默认为列主矩阵，
    matrix g_View;  // 可以在前面添加row_major表示行主矩阵
    matrix g_Proj;  // 该教程往后将使用默认的列主矩阵，但需要在C++代码端预先将矩阵进行转置。DirectXMath库XMATRIX是Row major的矩阵
                    // 而HLSL默认是Column major的矩阵，所以需要在C++端进行转置以匹配HLSL的矩阵布局。
}// MVP变换 ，这里proj全称 是projection matrix，投影矩阵


struct VertexIn
{
    float3 posL : POSITION;
    float4 color : COLOR;
};

struct VertexOut
{
    float4 posH : SV_POSITION;  // system value，表示顶点变换后的齐次坐标，供后续的光栅化阶段使用
    float4 color : COLOR;       // 注意HLSL中 ： 指的是“绑定”或“打标签”，非继承,也包括了cbuffer ConstantBuffer : register(b0) 这行
};
