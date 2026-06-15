// ============================================================
// Vertex.h — 顶点结构体及对应 D3D11 输入布局定义
// ============================================================
// 本文件集中定义了在渲染管线中需要用到的各种顶点格式，
// 每个结构体都包含：
//   1. 六大特殊成员函数（默认构造 / 拷贝构造 / 拷贝赋值 /
//      移动构造 / 移动赋值），均显式声明为 =default
//   2. 一个方便初始化的 constexpr 构造函数
//   3. 顶点数据成员（将被送入 VS Input Assembler）
//   4. 一个 static const D3D11_INPUT_ELEMENT_DESC 数组，
//      用于向 D3D11 描述该顶点结构的内存布局
// ============================================================

#ifndef VERTEX_H
#define VERTEX_H

#include <d3d11_1.h>
#include <DirectXMath.h>

// ----------------------------------------------------------
// 结构体：VertexPos
// 用途：仅包含位置信息的顶点（常用于纯色或调试几何体）
// 输入布局元素数量：1（位置）
// ----------------------------------------------------------
struct VertexPos
{
    VertexPos() = default;

    VertexPos(const VertexPos&) = default;
    VertexPos& operator=(const VertexPos&) = default;

    VertexPos(VertexPos&&) = default;
    VertexPos& operator=(VertexPos&&) = default;

    // 用位置向量构造顶点，constexpr 常量表达式
    // constexpr 关键字允许在编译期进行常量表达式的初始化，适用于顶点数据通常是静态常量的情况
    constexpr VertexPos(const DirectX::XMFLOAT3& _pos) : pos(_pos) {}

    DirectX::XMFLOAT3 pos;                        // 顶点位置 (x, y, z)
    static const D3D11_INPUT_ELEMENT_DESC inputLayout[1]; // 输入布局描述（仅 SV_Position）
};

// ----------------------------------------------------------
// 结构体：VertexPosColor
// 用途：包含位置和颜色的顶点（常用于顶点着色器直接输出颜色）
// 输入布局元素数量：2（位置 + 颜色）
// ----------------------------------------------------------
struct VertexPosColor
{
    VertexPosColor() = default;

    VertexPosColor(const VertexPosColor&) = default;
    VertexPosColor& operator=(const VertexPosColor&) = default;

    VertexPosColor(VertexPosColor&&) = default;
    VertexPosColor& operator=(VertexPosColor&&) = default;

    // 用位置和颜色向量构造顶点
    constexpr VertexPosColor(const DirectX::XMFLOAT3& _pos, const DirectX::XMFLOAT4& _color) :
        pos(_pos), color(_color) {}

    DirectX::XMFLOAT3 pos;                        // 顶点位置 (x, y, z)
    DirectX::XMFLOAT4 color;                      // 顶点颜色 (r, g, b, a)
    static const D3D11_INPUT_ELEMENT_DESC inputLayout[2];
};

// ----------------------------------------------------------
// 结构体：VertexPosTex
// 用途：包含位置和纹理坐标的顶点（用于纹理映射，不带光照）
// 输入布局元素数量：2（位置 + 纹理坐标）
// ----------------------------------------------------------
struct VertexPosTex
{
    VertexPosTex() = default;

    VertexPosTex(const VertexPosTex&) = default;
    VertexPosTex& operator=(const VertexPosTex&) = default;

    VertexPosTex(VertexPosTex&&) = default;
    VertexPosTex& operator=(VertexPosTex&&) = default;

    // 用位置和纹理坐标构造顶点
    constexpr VertexPosTex(const DirectX::XMFLOAT3& _pos, const DirectX::XMFLOAT2& _tex) :
        pos(_pos), tex(_tex) {}

    DirectX::XMFLOAT3 pos;                        // 顶点位置 (x, y, z)
    DirectX::XMFLOAT2 tex;                        // 纹理坐标 (u, v)
    static const D3D11_INPUT_ELEMENT_DESC inputLayout[2];
};

// ----------------------------------------------------------
// 结构体：VertexPosSize
// 用途：包含位置和二维尺寸的顶点（常用于粒子/公告板系统）
//       在 VS 中根据尺寸和世界坐标生成实际 Billboard 四边形
// 输入布局元素数量：2（位置 + 尺寸）
// ----------------------------------------------------------
struct VertexPosSize
{
    VertexPosSize() = default;

    VertexPosSize(const VertexPosSize&) = default;
    VertexPosSize& operator=(const VertexPosSize&) = default;

    VertexPosSize(VertexPosSize&&) = default;
    VertexPosSize& operator=(VertexPosSize&&) = default;

    // 用位置和尺寸向量构造顶点
    constexpr VertexPosSize(const DirectX::XMFLOAT3& _pos, const DirectX::XMFLOAT2& _size) :
        pos(_pos), size(_size) {}

    DirectX::XMFLOAT3 pos;                        // 中心位置 (x, y, z)
    DirectX::XMFLOAT2 size;                       // 二维尺寸 (width, height)
    static const D3D11_INPUT_ELEMENT_DESC inputLayout[2];
};

// ----------------------------------------------------------
// 结构体：VertexPosNormalColor
// 用途：包含位置、法线和颜色的顶点（用于带光照且顶点颜色可调的模型）
// 输入布局元素数量：3（位置 + 法线 + 颜色）
// ----------------------------------------------------------
struct VertexPosNormalColor
{
    VertexPosNormalColor() = default;

    VertexPosNormalColor(const VertexPosNormalColor&) = default;
    VertexPosNormalColor& operator=(const VertexPosNormalColor&) = default;

    VertexPosNormalColor(VertexPosNormalColor&&) = default;
    VertexPosNormalColor& operator=(VertexPosNormalColor&&) = default;

    // 用位置、法线和颜色向量构造顶点
    constexpr VertexPosNormalColor(const DirectX::XMFLOAT3& _pos, const DirectX::XMFLOAT3& _normal,
        const DirectX::XMFLOAT4& _color) :
        pos(_pos), normal(_normal), color(_color) {}

    DirectX::XMFLOAT3 pos;                        // 顶点位置 (x, y, z)
    DirectX::XMFLOAT3 normal;                     // 顶点法线 (nx, ny, nz) —— 用于光照计算
    DirectX::XMFLOAT4 color;                      // 顶点颜色 (r, g, b, a)
    static const D3D11_INPUT_ELEMENT_DESC inputLayout[3];
};


// ----------------------------------------------------------
// 结构体：VertexPosNormalTex
// 用途：包含位置、法线和纹理坐标的顶点（标准光照 + 纹理映射）
//       这是最常见的有光照模型顶点格式之一
// 输入布局元素数量：3（位置 + 法线 + 纹理坐标）
// ----------------------------------------------------------
struct VertexPosNormalTex
{
    VertexPosNormalTex() = default;

    VertexPosNormalTex(const VertexPosNormalTex&) = default;
    VertexPosNormalTex& operator=(const VertexPosNormalTex&) = default;

    VertexPosNormalTex(VertexPosNormalTex&&) = default;
    VertexPosNormalTex& operator=(VertexPosNormalTex&&) = default;

    // 用位置、法线和纹理坐标构造顶点
    constexpr VertexPosNormalTex(const DirectX::XMFLOAT3& _pos, const DirectX::XMFLOAT3& _normal,
        const DirectX::XMFLOAT2& _tex) :
        pos(_pos), normal(_normal), tex(_tex) {}

    DirectX::XMFLOAT3 pos;                        // 顶点位置 (x, y, z)
    DirectX::XMFLOAT3 normal;                     // 顶点法线 (nx, ny, nz) —— 用于光照计算
    DirectX::XMFLOAT2 tex;                        // 纹理坐标 (u, v)
    static const D3D11_INPUT_ELEMENT_DESC inputLayout[3];
};

// ----------------------------------------------------------
// 结构体：VertexPosNormalTangentTex
// 用途：包含位置、法线、切线方向和纹理坐标的顶点
//       切线用于法线贴图（Normal Mapping）等切线空间运算
// 输入布局元素数量：4（位置 + 法线 + 切线 + 纹理坐标）
// ----------------------------------------------------------
struct VertexPosNormalTangentTex
{
    // 1. 默认构造函数 = default，允许编译器生成最经济的默认构造行为
    VertexPosNormalTangentTex() = default;

    // 2. 显式声明拷贝构造函数和拷贝赋值运算符
    VertexPosNormalTangentTex(const VertexPosNormalTangentTex&) = default;
    VertexPosNormalTangentTex& operator=(const VertexPosNormalTangentTex&) = default;

    // 3. 显式声明移动构造函数和移动赋值运算符（C++11 移动语义，提高性能）
    VertexPosNormalTangentTex(VertexPosNormalTangentTex&&) = default;
    VertexPosNormalTangentTex& operator=(VertexPosNormalTangentTex&&) = default;

    // 4. constexpr 构造函数：允许在编译期进行常量表达式的初始化
    constexpr VertexPosNormalTangentTex(const DirectX::XMFLOAT3& _pos, const DirectX::XMFLOAT3& _normal,
        const DirectX::XMFLOAT4& _tangent, const DirectX::XMFLOAT2& _tex) :
        pos(_pos), normal(_normal), tangent(_tangent), tex(_tex) {}

    // 5. 成员变量：这些数据将被打包送往 GPU 的显存
    DirectX::XMFLOAT3 pos;      // 位置 (X, Y, Z) - 大小：12 字节
    DirectX::XMFLOAT3 normal;   // 法线方向 (X, Y, Z) - 大小：12 字节
    DirectX::XMFLOAT4 tangent;  // 切线方向 (X, Y, Z, W) - W 分量用于指示副切线方向 (±1) - 大小：16 字节
    DirectX::XMFLOAT2 tex;      // 纹理坐标 (U, V) - 大小：8 字节

    // 6. 静态常量数组声明：用来向 D3D 描述这个结构体的内存布局
    static const D3D11_INPUT_ELEMENT_DESC inputLayout[4];
};

#endif