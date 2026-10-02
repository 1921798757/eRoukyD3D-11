//***************************************************************************************
// MeshData.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 存放网格数据
// Mesh data storage.
//
// 【文件作用】
//   MeshData 描述"一个子网格(submesh)"在 GPU 侧的全部几何数据。
//   一个模型(Model)通常由多个 MeshData 组成（例如一个角色模型 = 身体 + 头发 + 衣服...）。
//
// 【核心设计思想 —— 分离式顶点缓冲】
//   传统做法是把一个顶点的所有属性打包成一个结构体（交错格式, 如
//   struct Vertex { pos, normal, uv; }），然后放进一个大顶点缓冲。
//   而这里采用"按属性拆分"的方式：位置、法线、UV、切线……各自独立成一个
//   ID3D11Buffer。这样做的好处是：
//     1. 灵活 —— 不同的着色器(Effect)需要不同的顶点属性组合，按需取用即可；
//     2. 复用 —— 同一份位置缓冲可被多个渲染通道共享；
//     3. 与本项目 EffectHelper / IEffectMeshData 的"动态输入布局"机制天然契合。
//   代价是绑定时需要多次 IASetVertexBuffers（见 GameObject::Draw）。
//***************************************************************************************

#pragma once

#ifndef MESH_DATA_H
#define MESH_DATA_H

#include <wrl/client.h>          // Microsoft::WRL::ComPtr —— COM 智能指针
#include <vector>
#include <DirectXCollision.h>    // DirectX::BoundingBox —— 包围盒/视锥剔除

// 前向声明，避免包含整个 d3d11.h，减少编译依赖
struct ID3D11Buffer;

// 一个子网格的全部 GPU 数据与元信息
struct MeshData
{
    // 使用模板别名(C++11)简化类型名
    // 这样下面写 ComPtr<ID3D11Buffer> 就等价于 Microsoft::WRL::ComPtr<ID3D11Buffer>
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    // ==================== 顶点属性缓冲（每个都是独立的 GPU 顶点缓冲） ====================
    // 下面这些指针指向显存中的 ID3D11Buffer，渲染时通过 IASetVertexBuffers 绑定。
    // 若某项为空(即 Get() 返回 nullptr)，表示该网格没有这种属性（例如没有顶点色）。

    ComPtr<ID3D11Buffer> m_pVertices;    // 顶点位置数组，元素通常为 XMFLOAT3。必有。
    ComPtr<ID3D11Buffer> m_pNormals;     // 法线数组，元素为 XMFLOAT3。用于光照计算。
    std::vector<ComPtr<ID3D11Buffer>> m_pTexcoordArrays;
                                         // 纹理坐标(UV)数组，元素为 XMFLOAT2。
                                         // 用 vector 是因为一个网格可以有多套 UV（最多 8 套），
                                         // 例如光照贴图 UV 和普通贴图 UV 分开存放。
    ComPtr<ID3D11Buffer> m_pTangents;    // 切线数组，元素为 XMFLOAT4。用于法线贴图(切线空间)。
    ComPtr<ID3D11Buffer> m_pBitangents;  // 副切线(双切线)数组。与法线、切线共同构成 TBN 矩阵。
    ComPtr<ID3D11Buffer> m_pColors;      // 顶点颜色数组。可选，很多模型没有顶点色。

    // ==================== 索引缓冲 ====================
    ComPtr<ID3D11Buffer> m_pIndices;     // 索引数组，描述三角形如何由顶点组成。
                                         // 可能是 16 位(顶点数<65535)或 32 位索引，
                                         // 具体格式在绑定时由 m_IndexCount 判断（见 GameObject::Draw）。

    // ==================== 元信息 ====================
    uint32_t m_VertexCount = 0;          // 顶点总数（用于需要按顶点数遍历的场合）
    uint32_t m_IndexCount = 0;           // 索引总数。DrawIndexed 绘制时用它决定画多少个三角形。
                                         // 三角形个数 = m_IndexCount / 3。
    uint32_t m_MaterialIndex = 0;        // 本子网格使用的材质索引，指向 Model::materials[m_MaterialIndex]。
                                         // 一个模型里不同子网格可以用不同材质（如皮肤 vs 衣服）。

    // ==================== 剔除相关 ====================
    DirectX::BoundingBox m_BoundingBox;  // 该子网格的轴对齐包围盒(AABB)，局部空间。
                                         // 用于视锥剔除和碰撞检测（见 GameObject::FrustumCulling）。
    bool m_InFrustum = true;             // 标记该子网格当前是否在摄像机视锥体内。
                                         // true 表示可见需要绘制；剔除后为 false 则跳过绘制。
};





#endif
