
#ifndef CONSTANT_BUFFERS_HLSL
#define CONSTANT_BUFFERS_HLSL

// 每个物体（每次绘制）都要重新设置的常量：各种矩阵组合
// 名字虽然叫 "EveryInstanceDrawing"，实际是按物体 set 的
cbuffer CBChangesEveryInstanceDrawing : register(b0)
{
    // 法线变换矩阵 = transpose(inverse(World)) * View
    // 把法线从局部空间变到视图空间。法线不能用 World 直接变换，
    // 否则在非等比缩放（拉伸/斜切）下会算歪
    matrix g_WorldInvTransposeView;
    // 世界视图矩阵 = World * View，把顶点位置从局部空间变到视图空间（相机空间）
    matrix g_WorldView;
    // 视图矩阵的逆 = inverse(View)，把方向/位置从视图空间变回世界空间，
    // 光照在世界空间里算时会用到（例如把视图空间法线转回世界空间）
    matrix g_InvView;
    // 视图投影矩阵 = View * Proj，视图空间 -> 裁剪空间
    matrix g_ViewProj;
    // 投影矩阵（透视/正交），单独使用，例如从深度反投影重建位置
    matrix g_Proj;
    // 世界视图投影矩阵 = World * View * Proj，局部空间 -> 裁剪空间，
    // 顶点着色器最终输出到 SV_POSITION 用的就是它
    matrix g_WorldViewProj;
}

// 每帧设置一次的常量：相机参数 + 若干 UI 调试开关
cbuffer CBPerFrame : register(b1)
{
    // xy 分别存相机的近、远平面距离（x = nearZ, y = farZ），
    // 用于深度重建，以及判断像素是否落在有效深度范围内
    float4 g_CameraNearFar;
    // 调试开关：置 1 时把材质 albedo 强制为白色，只显示光照结果
    uint g_LightingOnly;
    // 调试开关：置 1 时用几何面法线（由 ddx/ddy 求导得到）代替插值的平滑法线
    uint g_FaceNormals;
    // 调试开关：置 1 时用颜色/亮度可视化每个像素累加了多少个光源，
    // 用来检查光照范围/剔除是否正确
    uint g_VisualizeLightCount;
    // 调试开关：置 1 时可视化 MSAA 逐采样着色，标出哪些采样点走了独立着色
    uint g_VisualizePerSampleShading;
}

#endif // CONSTANTBUFFERS_HLSL
