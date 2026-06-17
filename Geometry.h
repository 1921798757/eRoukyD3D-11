//***************************************************************************************
// 生成常见的几何体网格模型
//***************************************************************************************

#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <vector>
#include <string>
#include <map>
#include <functional>
#include "Vertex.h"

namespace Geometry
{
    // 网格数据
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    struct MeshData
    {
        std::vector<VertexType> vertexVec;	// 顶点数组
        std::vector<IndexType> indexVec;	// 索引数组

        MeshData()
        {
            // 需检验索引类型合法性
            static_assert(sizeof(IndexType) == 2 || sizeof(IndexType) == 4, "The size of IndexType must be 2 bytes or 4 bytes!");
            static_assert(std::is_unsigned<IndexType>::value, "IndexType must be unsigned integer!");
        }
    };

    // 创建球体网格数据，levels和slices越大，精度越高。
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateSphere(float radius = 1.0f, UINT levels = 20, UINT slices = 20,
        const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 创建立方体网格数据
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateBox(float width = 2.0f, float height = 2.0f, float depth = 2.0f,
        const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 创建圆柱体网格数据，slices越大，精度越高。
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateCylinder(float radius = 1.0f, float height = 2.0f, UINT slices = 20, UINT stacks = 10,
        float texU = 1.0f, float texV = 1.0f, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 创建只有圆柱体侧面的网格数据，slices越大，精度越高
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateCylinderNoCap(float radius = 1.0f, float height = 2.0f, UINT slices = 20, UINT stacks = 10,
        float texU = 1.0f, float texV = 1.0f, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 创建圆锥体网格数据，slices越大，精度越高。
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateCone(float radius = 1.0f, float height = 2.0f, UINT slices = 20,
        const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
    
        // 创建胶囊体网格数据（上下半球 + 柱体/锥台）
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateCapsule(float topRadius = 0.5f, float bottomRadius = 0.5f,
        float height = 2.0f, UINT slices = 20, UINT topLevels = 10, UINT bottomLevels = 10,
        const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 创建只有圆锥体侧面网格数据，slices越大，精度越高。
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateConeNoCap(float radius = 1.0f, float height = 2.0f, UINT slices = 20,
        const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 创建一个指定NDC屏幕区域的面(默认全屏)
    template<class VertexType = VertexPosTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> Create2DShow(const DirectX::XMFLOAT2& center, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
    template<class VertexType = VertexPosTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> Create2DShow(float centerX = 0.0f, float centerY = 0.0f, float scaleX = 1.0f, float scaleY = 1.0f, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 创建一个平面
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreatePlane(const DirectX::XMFLOAT2& planeSize, 
        const DirectX::XMFLOAT2& maxTexCoord = { 1.0f, 1.0f }, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreatePlane(float width = 10.0f, float depth = 10.0f, float texU = 1.0f, float texV = 1.0f,
        const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 创建一个地形
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateTerrain(const DirectX::XMFLOAT2& terrainSize,
        const DirectX::XMUINT2& slices = { 10, 10 }, const DirectX::XMFLOAT2 & maxTexCoord = { 1.0f, 1.0f },
        const std::function<float(float, float)>& heightFunc = [](float x, float z) { return 0.0f; },
        const std::function<DirectX::XMFLOAT3(float, float)>& normalFunc = [](float x, float z) { return XMFLOAT3(0.0f, 1.0f, 0.0f); },
        const std::function<DirectX::XMFLOAT4(float, float)>& colorFunc = [](float x, float z) { return XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); });
    template<class VertexType = VertexPosNormalTex, class IndexType = DWORD>
    MeshData<VertexType, IndexType> CreateTerrain(float width = 10.0f, float depth = 10.0f,
        UINT slicesX = 10, UINT slicesZ = 10, float texU = 1.0f, float texV = 1.0f,
        const std::function<float(float, float)>& heightFunc = [](float x, float z) { return 0.0f; },
        const std::function<DirectX::XMFLOAT3(float, float)>& normalFunc = [](float x, float z) { return XMFLOAT3(0.0f, 1.0f, 0.0f); },
        const std::function<DirectX::XMFLOAT4(float, float)>& colorFunc = [](float x, float z) { return XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); });
}






namespace Geometry
{
    namespace Internal
    {
        //
        // 以下结构体和函数仅供内部实现使用
        //

        // 内部顶点数据结构：包含所有可能的顶点属性（位置、法线、切线、颜色、纹理坐标），
        // 供 InsertVertexElement 从中选择性复制到目标顶点类型。
        struct VertexData
        {
            DirectX::XMFLOAT3 pos;
            DirectX::XMFLOAT3 normal;
            DirectX::XMFLOAT4 tangent;
            DirectX::XMFLOAT4 color;
            DirectX::XMFLOAT2 tex;
        };

        // 根据目标顶点类型 VertexType 的 inputLayout 语义信息，
        // 将 vertexSrc 中对应范围的字节数据拷贝到 vertexDst 中对应偏移处。
        // 这是实现"通用顶点生成"的关键：无论目标顶点格式是什么，
        // 只要其 inputLayout 包含 POSITION / NORMAL / TANGENT / COLOR / TEXCOORD 中的部分或全部语义，
        // 此函数就能正确地将数据填入对应的成员位置。
        template<class VertexType>
        inline void InsertVertexElement(VertexType& vertexDst, const VertexData& vertexSrc)
        {
            static std::string semanticName;
            static const std::map<std::string, std::pair<size_t, size_t>> semanticSizeMap = {
                {"POSITION", std::pair<size_t, size_t>(0, 12)},     // 0~12
                {"NORMAL", std::pair<size_t, size_t>(12, 24)},      // 12~24
                {"TANGENT", std::pair<size_t, size_t>(24, 40)},     // 24~40
                {"COLOR", std::pair<size_t, size_t>(40, 56)},       // 40~56
                {"TEXCOORD", std::pair<size_t, size_t>(56, 64)}     // 56~64
            };
            // ARRYSIZE:在编译期，计算出一个普通 C 风格数组里有多少个元素。
            for (size_t i = 0; i < ARRAYSIZE(VertexType::inputLayout); i++)
            {
                semanticName = VertexType::inputLayout[i].SemanticName;
                const auto& range = semanticSizeMap.at(semanticName);
                memcpy_s(reinterpret_cast<char*>(&vertexDst) + VertexType::inputLayout[i].AlignedByteOffset,
                    range.second - range.first,
                    reinterpret_cast<const char*>(&vertexSrc) + range.first,
                    range.second - range.first);
            }
        }
    }
    
    //
    // 几何体方法的实现
    //

    // ----------------------------------------------------------
    // CreateSphere — 创建球体网格
    // 参数：
    //   radius : 球体半径
    //   levels : 纬度分割数（越多越圆滑）
    //   slices : 经度分割数（越多越圆滑）
    //   color  : 顶点颜色
    // 说明：
    //   球体几何由经纬度网格构成，先放顶端点（北极），
    //   然后逐层生成中间纬圈顶点，最后放底端点（南极）。
    //   同时为每个顶点计算法线（归一化位置向量）和切线。
    // ----------------------------------------------------------
    template<class VertexType, class IndexType>
    inline MeshData<VertexType, IndexType> CreateSphere(float radius, UINT levels, UINT slices, const DirectX::XMFLOAT4 & color)
    {
        using namespace DirectX;

        MeshData<VertexType, IndexType> meshData;
        UINT vertexCount = 2 + (levels - 1) * (slices + 1);
        UINT indexCount = 6 * (levels - 1) * slices;
        meshData.vertexVec.resize(vertexCount);
        meshData.indexVec.resize(indexCount);

        Internal::VertexData vertexData;
        IndexType vIndex = 0, iIndex = 0;

        float phi = 0.0f, theta = 0.0f;
        float per_phi = XM_PI / levels;
        float per_theta = XM_2PI / slices;
        float x, y, z;

        // 放入顶端点
        vertexData = { XMFLOAT3(0.0f, radius, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(0.0f, 0.0f) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        for (UINT i = 1; i < levels; ++i)
        {
            phi = per_phi * i;
            // 需要slices + 1个顶点是因为 起点和终点需为同一点，但纹理坐标值不一致
            for (UINT j = 0; j <= slices; ++j)
            {
                theta = per_theta * j;
                x = radius * sinf(phi) * cosf(theta);
                y = radius * cosf(phi);
                z = radius * sinf(phi) * sinf(theta);
                // 计算出局部坐标、法向量、Tangent向量和纹理坐标
                XMFLOAT3 pos = XMFLOAT3(x, y, z), normal;
                XMStoreFloat3(&normal, XMVector3Normalize(XMLoadFloat3(&pos)));

                vertexData = { pos, normal, XMFLOAT4(-sinf(theta), 0.0f, cosf(theta), 1.0f), color, XMFLOAT2(theta / XM_2PI, phi / XM_PI) };
                Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);
            }
        }

        // 放入底端点
        vertexData = { XMFLOAT3(0.0f, -radius, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f),
            XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(0.0f, 1.0f) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);


        // 放入索引
        if (levels > 1)
        {
            for (UINT j = 1; j <= slices; ++j)
            {
                meshData.indexVec[iIndex++] = 0;
                meshData.indexVec[iIndex++] = j % (slices + 1) + 1;
                meshData.indexVec[iIndex++] = j;
            }
        }


        for (UINT i = 1; i < levels - 1; ++i)
        {
            for (UINT j = 1; j <= slices; ++j)
            {
                meshData.indexVec[iIndex++] = (i - 1) * (slices + 1) + j;
                meshData.indexVec[iIndex++] = (i - 1) * (slices + 1) + j % (slices + 1) + 1;
                meshData.indexVec[iIndex++] = i * (slices + 1) + j % (slices + 1) + 1;

                meshData.indexVec[iIndex++] = i * (slices + 1) + j % (slices + 1) + 1;
                meshData.indexVec[iIndex++] = i * (slices + 1) + j;
                meshData.indexVec[iIndex++] = (i - 1) * (slices + 1) + j;
            }
        }

        // 逐渐放入索引
        if (levels > 1)
        {
            for (UINT j = 1; j <= slices; ++j)
            {
                meshData.indexVec[iIndex++] = (levels - 2) * (slices + 1) + j;
                meshData.indexVec[iIndex++] = (levels - 2) * (slices + 1) + j % (slices + 1) + 1;
                meshData.indexVec[iIndex++] = (levels - 1) * (slices + 1) + 1;
            }
        }


        return meshData;
    }

    // ----------------------------------------------------------
    // CreateBox — 创建立方体网格
    // 参数：
    //   width  : X 轴方向宽度
    //   height : Y 轴方向高度
    //   depth  : Z 轴方向深度
    //   color  : 顶点颜色
    // 说明：
    //   立方体由 6 个面构成（右/左/顶/底/背/正），
    //   每个面由 4 个顶点组成，共 24 个顶点（因为每个面需要独立的法线/切线/纹理坐标）。
    //   索引数组直接列出 12 个三角形（每面 2 个）。
    // ----------------------------------------------------------
    template<class VertexType, class IndexType>
    inline MeshData<VertexType, IndexType> CreateBox(float width, float height, float depth, const DirectX::XMFLOAT4 & color)
    {
        using namespace DirectX;

        MeshData<VertexType, IndexType> meshData;
        meshData.vertexVec.resize(24);


        Internal::VertexData vertexDataArr[24];
        float w2 = width / 2, h2 = height / 2, d2 = depth / 2;

        // 右面(+X面)
        vertexDataArr[0].pos = XMFLOAT3(w2, -h2, -d2);
        vertexDataArr[1].pos = XMFLOAT3(w2, h2, -d2);
        vertexDataArr[2].pos = XMFLOAT3(w2, h2, d2);
        vertexDataArr[3].pos = XMFLOAT3(w2, -h2, d2);
        // 左面(-X面)
        vertexDataArr[4].pos = XMFLOAT3(-w2, -h2, d2);
        vertexDataArr[5].pos = XMFLOAT3(-w2, h2, d2);
        vertexDataArr[6].pos = XMFLOAT3(-w2, h2, -d2);
        vertexDataArr[7].pos = XMFLOAT3(-w2, -h2, -d2);
        // 顶面(+Y面)
        vertexDataArr[8].pos = XMFLOAT3(-w2, h2, -d2);
        vertexDataArr[9].pos = XMFLOAT3(-w2, h2, d2);
        vertexDataArr[10].pos = XMFLOAT3(w2, h2, d2);
        vertexDataArr[11].pos = XMFLOAT3(w2, h2, -d2);
        // 底面(-Y面)
        vertexDataArr[12].pos = XMFLOAT3(w2, -h2, -d2);
        vertexDataArr[13].pos = XMFLOAT3(w2, -h2, d2);
        vertexDataArr[14].pos = XMFLOAT3(-w2, -h2, d2);
        vertexDataArr[15].pos = XMFLOAT3(-w2, -h2, -d2);
        // 背面(+Z面)
        vertexDataArr[16].pos = XMFLOAT3(w2, -h2, d2);
        vertexDataArr[17].pos = XMFLOAT3(w2, h2, d2);
        vertexDataArr[18].pos = XMFLOAT3(-w2, h2, d2);
        vertexDataArr[19].pos = XMFLOAT3(-w2, -h2, d2);
        // 正面(-Z面)
        vertexDataArr[20].pos = XMFLOAT3(-w2, -h2, -d2);
        vertexDataArr[21].pos = XMFLOAT3(-w2, h2, -d2);
        vertexDataArr[22].pos = XMFLOAT3(w2, h2, -d2);
        vertexDataArr[23].pos = XMFLOAT3(w2, -h2, -d2);

        for (UINT i = 0; i < 4; ++i)
        {
            // 右面(+X面)
            vertexDataArr[i].normal = XMFLOAT3(1.0f, 0.0f, 0.0f);
            vertexDataArr[i].tangent = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
            vertexDataArr[i].color = color;
            // 左面(-X面)
            vertexDataArr[i + 4].normal = XMFLOAT3(-1.0f, 0.0f, 0.0f);
            vertexDataArr[i + 4].tangent = XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f);
            vertexDataArr[i + 4].color = color;
            // 顶面(+Y面)
            vertexDataArr[i + 8].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
            vertexDataArr[i + 8].tangent = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
            vertexDataArr[i + 8].color = color;
            // 底面(-Y面)
            vertexDataArr[i + 12].normal = XMFLOAT3(0.0f, -1.0f, 0.0f);
            vertexDataArr[i + 12].tangent = XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f);
            vertexDataArr[i + 12].color = color;
            // 背面(+Z面)
            vertexDataArr[i + 16].normal = XMFLOAT3(0.0f, 0.0f, 1.0f);
            vertexDataArr[i + 16].tangent = XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f);
            vertexDataArr[i + 16].color = color;
            // 正面(-Z面)
            vertexDataArr[i + 20].normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
            vertexDataArr[i + 20].tangent = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
            vertexDataArr[i + 20].color = color;
        }

        for (UINT i = 0; i < 6; ++i)
        {
            vertexDataArr[i * 4].tex = XMFLOAT2(0.0f, 1.0f);
            vertexDataArr[i * 4 + 1].tex = XMFLOAT2(0.0f, 0.0f);
            vertexDataArr[i * 4 + 2].tex = XMFLOAT2(1.0f, 0.0f);
            vertexDataArr[i * 4 + 3].tex = XMFLOAT2(1.0f, 1.0f);
        }

        for (UINT i = 0; i < 24; ++i)
        {
            Internal::InsertVertexElement(meshData.vertexVec[i], vertexDataArr[i]);
        }

        meshData.indexVec = {
            0, 1, 2, 2, 3, 0,		// 右面(+X面)
            4, 5, 6, 6, 7, 4,		// 左面(-X面)
            8, 9, 10, 10, 11, 8,	// 顶面(+Y面)
            12, 13, 14, 14, 15, 12,	// 底面(-Y面)
            16, 17, 18, 18, 19, 16, // 背面(+Z面)
            20, 21, 22, 22, 23, 20	// 正面(-Z面)
        };

        return meshData;
    }

    // ----------------------------------------------------------
    // CreateCylinder — 创建完整圆柱体网格（含顶盖和底盖）
    // 参数：
    //   radius : 圆柱半径
    //   height : 圆柱高度
    //   slices : 圆周分割数（越多越圆滑）
    //   stacks : 高度方向分割数
    //   texU, texV : 纹理坐标缩放
    //   color  : 顶点颜色
    // 说明：
    //   先调用 CreateCylinderNoCap 生成侧面，再额外添加顶部和底部的圆形面。
    //   顶部和底部分别由圆心顶点和圆周上的顶点构成扇形三角形网格。
    // ----------------------------------------------------------
    template<class VertexType, class IndexType>
    inline MeshData<VertexType, IndexType> CreateCylinder(float radius, float height, UINT slices, UINT stacks,
        float texU, float texV, const DirectX::XMFLOAT4 & color)
    {
        using namespace DirectX;

        auto meshData = CreateCylinderNoCap<VertexType, IndexType>(radius, height, slices, stacks, texU, texV, color);
        UINT vertexCount = (slices + 1) * (stacks + 3) + 2;
        UINT indexCount = 6 * slices * (stacks + 1);
        meshData.vertexVec.resize(vertexCount);
        meshData.indexVec.resize(indexCount);

        float h2 = height / 2;
        float theta = 0.0f;
        float per_theta = XM_2PI / slices;

        IndexType vIndex = (slices + 1) * (stacks + 1), iIndex = 6 * slices * stacks;
        IndexType offset = vIndex;
        Internal::VertexData vertexData;

        // 放入顶端圆心
        vertexData = { XMFLOAT3(0.0f, h2, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f),
            XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(0.5f, 0.5f) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        // 放入顶端圆上各点
        for (UINT i = 0; i <= slices; ++i)
        {
            theta = i * per_theta;
            float u = cosf(theta) * radius / height + 0.5f;
            float v = sinf(theta) * radius / height + 0.5f;
            vertexData = { XMFLOAT3(radius * cosf(theta), h2, radius * sinf(theta)), XMFLOAT3(0.0f, 1.0f, 0.0f),
                XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(u, v)  };
            Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);
        }

        // 放入底端圆心
        vertexData = { XMFLOAT3(0.0f, -h2, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f),
            XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(0.5f, 0.5f) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        // 放入底部圆上各点
        for (UINT i = 0; i <= slices; ++i)
        {
            theta = i * per_theta;
            float u = cosf(theta) * radius / height + 0.5f;
            float v = sinf(theta) * radius / height + 0.5f;
            vertexData = { XMFLOAT3(radius * cosf(theta), -h2, radius * sinf(theta)), XMFLOAT3(0.0f, -1.0f, 0.0f),
                XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(u, v) };
            Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);
        }

        

        // 放入顶部三角形索引
        for (UINT i = 1; i <= slices; ++i)
        {
            meshData.indexVec[iIndex++] = offset;
            meshData.indexVec[iIndex++] = offset + i % (slices + 1) + 1;
            meshData.indexVec[iIndex++] = offset + i;
        }

        // 放入底部三角形索引
        offset += slices + 2;
        for (UINT i = 1; i <= slices; ++i)
        {
            meshData.indexVec[iIndex++] = offset;
            meshData.indexVec[iIndex++] = offset + i;
            meshData.indexVec[iIndex++] = offset + i % (slices + 1) + 1;
        }

        return meshData;
    }

    
    // ----------------------------------------------------------
    // CreateCylinderNoCap — 创建圆柱体侧面网格（无顶盖和底盖）
    // 参数：
    //   radius : 圆柱半径
    //   height : 圆柱高度
    //   slices : 圆周分割数
    //   stacks : 高度方向分割数
    //   texU, texV : 纹理坐标缩放
    //   color  : 顶点颜色
    // 说明：
    //   按照"自底向上"的顺序逐层生成侧面顶点，
    //   每层包含 (slices + 1) 个顶点（首尾重合以支持纹理环绕）。
    //   索引按两个三角形构成一个四边形网格的方式排列。
    // ----------------------------------------------------------
    template<class VertexType, class IndexType>
    inline MeshData<VertexType, IndexType> CreateCylinderNoCap(float radius, float height, UINT slices, UINT stacks,
        float texU, float texV, const DirectX::XMFLOAT4 & color)
    {
        using namespace DirectX;

        MeshData<VertexType, IndexType> meshData;
        UINT vertexCount = (slices + 1) * (stacks + 1);
        UINT indexCount = 6 * slices * stacks;
        meshData.vertexVec.resize(vertexCount);
        meshData.indexVec.resize(indexCount);

        float h2 = height / 2;
        float theta = 0.0f;
        float per_theta = XM_2PI / slices;
        float stackHeight = height / stacks;

        Internal::VertexData vertexData;

        // 自底向上铺设侧面端点
        UINT vIndex = 0;
        for (UINT i = 0; i < stacks + 1; ++i)
        {
            float y = -h2 + i * stackHeight;
            // 当前层顶点
            for (UINT j = 0; j <= slices; ++j)
            {
                theta = j * per_theta;
                float u = theta / XM_2PI;
                float v = 1.0f - (float)i / stacks;
                vertexData = { XMFLOAT3(radius * cosf(theta), y, radius * sinf(theta)), XMFLOAT3(cosf(theta), 0.0f, sinf(theta)),
                    XMFLOAT4(-sinf(theta), 0.0f, cosf(theta), 1.0f), color, XMFLOAT2(u * texU, v * texV) };
                Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);
            }
        }

        // 放入索引
        UINT iIndex = 0;
        for (UINT i = 0; i < stacks; ++i)
        {
            for (UINT j = 0; j < slices; ++j)
            {
                meshData.indexVec[iIndex++] = i * (slices + 1) + j;
                meshData.indexVec[iIndex++] = (i + 1) * (slices + 1) + j;
                meshData.indexVec[iIndex++] = (i + 1) * (slices + 1) + j + 1;

                meshData.indexVec[iIndex++] = i * (slices + 1) + j;
                meshData.indexVec[iIndex++] = (i + 1) * (slices + 1) + j + 1;
                meshData.indexVec[iIndex++] = i * (slices + 1) + j + 1;
            }
        }
        


        return meshData;
    }

    // ----------------------------------------------------------
    // CreateCone — 创建完整圆锥体网格（含底面）
    // 参数：
    //   radius : 底面半径
    //   height : 圆锥高度
    //   slices : 圆周分割数
    //   color  : 顶点颜色
    // 说明：
    //   先调用 CreateConeNoCap 生成侧面，再添加底面圆形面。
    //   底面由圆心和圆周上的顶点构成扇形三角形网格。
    // ----------------------------------------------------------
    template<class VertexType, class IndexType>
    MeshData<VertexType, IndexType> CreateCone(float radius, float height, UINT slices, const DirectX::XMFLOAT4& color)
    {
        using namespace DirectX;
        auto meshData = CreateConeNoCap<VertexType, IndexType>(radius, height, slices, color);

        UINT vertexCount = 3 * slices + 1;
        UINT indexCount = 6 * slices;
        meshData.vertexVec.resize(vertexCount);
        meshData.indexVec.resize(indexCount);
        
        float h2 = height / 2;
        float theta = 0.0f;
        float per_theta = XM_2PI / slices;
        UINT iIndex = 3 * slices;
        UINT vIndex = 2 * slices;
        Internal::VertexData vertexData;

        // 放入圆锥底面顶点
        for (UINT i = 0; i < slices; ++i)
        {
            theta = i * per_theta;
            vertexData = { XMFLOAT3(radius * cosf(theta), -h2, radius * sinf(theta)), XMFLOAT3(0.0f, -1.0f, 0.0f),
                XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(cosf(theta) / 2 + 0.5f, sinf(theta) / 2 + 0.5f) };
            Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);
        }
        // 放入圆锥底面圆心
        vertexData = { XMFLOAT3(0.0f, -h2, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f),
                XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(0.5f, 0.5f) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        // 放入索引
        UINT offset = 2 * slices;
        for (UINT i = 0; i < slices; ++i)
        {
            meshData.indexVec[iIndex++] = offset + slices;
            meshData.indexVec[iIndex++] = offset + i % slices;
            meshData.indexVec[iIndex++] = offset + (i + 1) % slices;
        }

        return meshData;
    }


    // ----------------------------------------------------------
    // CreateCapsule — 创建胶囊体网格（上下半球 + 柱体/锥台）
    // 参数：
    //   topRadius    : 上半球半径
    //   bottomRadius : 下半球半径
    //   height       : 柱体部分高度（不含半球）
    //   slices       : 经度方向切片数（圆周分割）
    //   topLevels    : 上半球纬度层级数
    //   bottomLevels : 下半球纬度层级数
    //   color        : 顶点颜色
    // 说明：
    //   胶囊体由三部分组成：上半球（topLevels层）、柱体（height高度）、下半球（bottomLevels层）。
    //   当 topRadius != bottomRadius 时，中间连接部分为锥台（frustum）。
    //   在半球与柱体交界处复制顶点，以保证法线在棱线处正确断开。
    //   纹理坐标 v 轴按三部分的高度比例分配：0(北极) → 1(南极)。
    //   顶点包含位置、法线、切线、纹理坐标（由 VertexType 决定实际输出哪些属性）。
    // ----------------------------------------------------------
    template<class VertexType, class IndexType>
    inline MeshData<VertexType, IndexType> CreateCapsule(float topRadius, float bottomRadius,
        float height, UINT slices, UINT topLevels, UINT bottomLevels, const DirectX::XMFLOAT4& color)
    {
        using namespace DirectX;

        MeshData<VertexType, IndexType> meshData;

        float h2 = height / 2.0f;
        float per_theta = XM_2PI / slices;
        float per_phi_top = XM_PIDIV2 / topLevels;
        float per_phi_bot = XM_PIDIV2 / bottomLevels;

        // 纹理坐标 v 轴按三部分高度比例分配
        float totalH = topRadius + height + bottomRadius;
        float vCylBase = topRadius / totalH;           // 柱体顶部 v 值
        float vBotBase = (topRadius + height) / totalH; // 柱体底部 v 值

        // 柱体/锥台部分法线计算参数
        float dr = bottomRadius - topRadius;
        float slantLen = sqrtf(height * height + dr * dr);

        // 顶点数 = 北极(1) + 上半球topLevels环 + 柱体2环 + 下半球bottomLevels环 + 南极(1)
        UINT vertexCount = 2 + (topLevels + bottomLevels + 2) * (slices + 1);
        // 索引数 = 6 * (topLevels + bottomLevels) * slices
        UINT indexCount = 6 * (topLevels + bottomLevels) * slices;
        meshData.vertexVec.resize(vertexCount);
        meshData.indexVec.resize(indexCount);

        Internal::VertexData vertexData;
        UINT vIndex = 0, iIndex = 0;

        // ===================== 上半球 =====================

        // 北极顶点
        vertexData = { XMFLOAT3(0.0f, h2 + topRadius, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f),
            XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(0.0f, 0.0f) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        // 上半球纬度环 i = 1..topLevels（phi 从 0 到 π/2）
        for (UINT i = 1; i <= topLevels; ++i)
        {
            float phi = per_phi_top * i;
            float sinPhi = sinf(phi), cosPhi = cosf(phi);
            float v = vCylBase * (float)i / topLevels;

            for (UINT j = 0; j <= slices; ++j)
            {
                float theta = per_theta * j;
                float sinT = sinf(theta), cosT = cosf(theta);
                float x = topRadius * sinPhi * cosT;
                float y = h2 + topRadius * cosPhi;
                float z = topRadius * sinPhi * sinT;

                // 法线 = (pos - 球心) 归一化，球心在 (0, h2, 0)
                XMFLOAT3 normal;
                XMStoreFloat3(&normal, XMVector3Normalize(XMVectorSet(x, y - h2, z, 0.0f)));

                vertexData = { XMFLOAT3(x, y, z), normal, XMFLOAT4(-sinT, 0.0f, cosT, 1.0f),
                    color, XMFLOAT2(theta / XM_2PI, v) };
                Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);
            }
        }

        // ===================== 柱体/锥台 =====================

        // 柱体顶环 (y = +h2, 半径 = topRadius)
        for (UINT j = 0; j <= slices; ++j)
        {
            float theta = per_theta * j;
            float sinT = sinf(theta), cosT = cosf(theta);
            // 锥台法线：考虑上下半径差异导致的倾斜
            XMFLOAT3 normal(height * cosT / slantLen, dr / slantLen, height * sinT / slantLen);
            vertexData = { XMFLOAT3(topRadius * cosT, h2, topRadius * sinT), normal,
                XMFLOAT4(-sinT, 0.0f, cosT, 1.0f), color, XMFLOAT2(theta / XM_2PI, vCylBase) };
            Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);
        }

        // 柱体底环 (y = -h2, 半径 = bottomRadius)
        for (UINT j = 0; j <= slices; ++j)
        {
            float theta = per_theta * j;
            float sinT = sinf(theta), cosT = cosf(theta);
            XMFLOAT3 normal(height * cosT / slantLen, dr / slantLen, height * sinT / slantLen);
            vertexData = { XMFLOAT3(bottomRadius * cosT, -h2, bottomRadius * sinT), normal,
                XMFLOAT4(-sinT, 0.0f, cosT, 1.0f), color, XMFLOAT2(theta / XM_2PI, vBotBase) };
            Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);
        }

        // ===================== 下半球 =====================

        // 下半球纬度环 i = 0..bottomLevels-1（phi 从 π/2 到接近 π）
        // Ring 0 为下半球赤道（与柱体底环位置相同但法线不同），Ring bottomLevels-1 为最后一道中间环
        for (UINT i = 0; i < bottomLevels; ++i)
        {
            float phi = XM_PIDIV2 + per_phi_bot * i;
            float sinPhi = sinf(phi), cosPhi = cosf(phi);
            float v = vBotBase + (1.0f - vBotBase) * (float)i / bottomLevels;

            for (UINT j = 0; j <= slices; ++j)
            {
                float theta = per_theta * j;
                float sinT = sinf(theta), cosT = cosf(theta);
                float x = bottomRadius * sinPhi * cosT;
                float y = -h2 + bottomRadius * cosPhi;
                float z = bottomRadius * sinPhi * sinT;

                // 法线 = (pos - 球心) 归一化，球心在 (0, -h2, 0)
                XMFLOAT3 normal;
                XMStoreFloat3(&normal, XMVector3Normalize(XMVectorSet(x, y + h2, z, 0.0f)));

                vertexData = { XMFLOAT3(x, y, z), normal, XMFLOAT4(-sinT, 0.0f, cosT, 1.0f),
                    color, XMFLOAT2(theta / XM_2PI, v) };
                Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);
            }
        }

        // 南极顶点
        vertexData = { XMFLOAT3(0.0f, -h2 - bottomRadius, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f),
            XMFLOAT4(-1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(0.0f, 1.0f) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        // ===================== 索引生成 =====================

        // --- 上半球顶帽 (北极 → 环1) ---
        for (UINT j = 0; j < slices; ++j)
        {
            meshData.indexVec[iIndex++] = 0;
            meshData.indexVec[iIndex++] = 1 + j + 1;
            meshData.indexVec[iIndex++] = 1 + j;
        }

        // --- 上半球中间带 (环i → 环i+1), i = 1..topLevels-1 ---
        for (UINT i = 1; i < topLevels; ++i)
        {
            UINT ringCurr = 1 + (i - 1) * (slices + 1);
            UINT ringNext = 1 + i * (slices + 1);
            for (UINT j = 0; j < slices; ++j)
            {
                meshData.indexVec[iIndex++] = ringCurr + j;
                meshData.indexVec[iIndex++] = ringCurr + j + 1;
                meshData.indexVec[iIndex++] = ringNext + j + 1;

                meshData.indexVec[iIndex++] = ringCurr + j;
                meshData.indexVec[iIndex++] = ringNext + j + 1;
                meshData.indexVec[iIndex++] = ringNext + j;
            }
        }

        // --- 柱体/锥台 (顶环 → 底环) ---
        UINT cylTopStart = 1 + topLevels * (slices + 1);
        UINT cylBotStart = cylTopStart + (slices + 1);
        for (UINT j = 0; j < slices; ++j)
        {
            meshData.indexVec[iIndex++] = cylTopStart + j;
            meshData.indexVec[iIndex++] = cylTopStart + j + 1;
            meshData.indexVec[iIndex++] = cylBotStart + j + 1;

            meshData.indexVec[iIndex++] = cylTopStart + j;
            meshData.indexVec[iIndex++] = cylBotStart + j + 1;
            meshData.indexVec[iIndex++] = cylBotStart + j;
        }

        // --- 下半球中间带 (环i → 环i+1), i = 0..bottomLevels-2 ---
        // 下半球环 0（赤道）起始索引
        UINT botEquatorStart = cylBotStart + (slices + 1);
        for (UINT i = 0; i + 1 < bottomLevels; ++i)
        {
            UINT ringCurr = botEquatorStart + i * (slices + 1);
            UINT ringNext = botEquatorStart + (i + 1) * (slices + 1);
            for (UINT j = 0; j < slices; ++j)
            {
                meshData.indexVec[iIndex++] = ringCurr + j;
                meshData.indexVec[iIndex++] = ringCurr + j + 1;
                meshData.indexVec[iIndex++] = ringNext + j + 1;

                meshData.indexVec[iIndex++] = ringCurr + j;
                meshData.indexVec[iIndex++] = ringNext + j + 1;
                meshData.indexVec[iIndex++] = ringNext + j;
            }
        }

        // --- 下半球底帽 (最后一环 → 南极) ---
        UINT southPole = vIndex - 1;
        UINT lastRingStart = botEquatorStart + (bottomLevels - 1) * (slices + 1);
        for (UINT j = 0; j < slices; ++j)
        {
            meshData.indexVec[iIndex++] = lastRingStart + j;
            meshData.indexVec[iIndex++] = lastRingStart + j + 1;
            meshData.indexVec[iIndex++] = southPole;
        }

        return meshData;
    }

    // ----------------------------------------------------------
    // CreateConeNoCap — 创建圆锥体侧面网格（无底面）
    // 参数：
    //   radius : 底面半径
    //   height : 圆锥高度
    //   slices : 圆周分割数
    //   color  : 顶点颜色
    // 说明：
    //   顶点分为两组：前 slices 个为尖端顶点（位置相同但法线/切线不同），
    //   后 slices 个为底部圆周顶点。每个三角形由尖端的一个顶点和底部相邻两个顶点构成。
    //   法线沿圆锥斜面方向（由底部指向尖端）。
    // ----------------------------------------------------------
    template<class VertexType, class IndexType>
    MeshData<VertexType, IndexType> CreateConeNoCap(float radius, float height, UINT slices, const DirectX::XMFLOAT4& color)
    {
        using namespace DirectX;

        MeshData<VertexType, IndexType> meshData;
        UINT vertexCount = 2 * slices;
        UINT indexCount = 3 * slices;
        meshData.vertexVec.resize(vertexCount);
        meshData.indexVec.resize(indexCount);

        float h2 = height / 2;
        float theta = 0.0f;
        float per_theta = XM_2PI / slices;
        float len = sqrtf(height * height + radius * radius);
        UINT iIndex = 0;
        UINT vIndex = 0;
        Internal::VertexData vertexData;

        // 放入圆锥尖端顶点(每个顶点位置相同，但包含不同的法向量和切线向量)
        for (UINT i = 0; i < slices; ++i)
        {
            theta = i * per_theta + per_theta / 2;
            vertexData = { XMFLOAT3(0.0f, h2, 0.0f), XMFLOAT3(radius * cosf(theta) / len, height / len, radius * sinf(theta) / len),
                XMFLOAT4(-sinf(theta), 0.0f, cosf(theta), 1.0f), color, XMFLOAT2(0.5f, 0.5f) };
            Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);
        }

        // 放入圆锥侧面底部顶点
        for (UINT i = 0; i < slices; ++i)
        {
            theta = i * per_theta;
            vertexData = { XMFLOAT3(radius * cosf(theta), -h2, radius * sinf(theta)), XMFLOAT3(radius * cosf(theta) / len, height / len, radius * sinf(theta) / len),
                XMFLOAT4(-sinf(theta), 0.0f, cosf(theta), 1.0f), color, XMFLOAT2(cosf(theta) / 2 + 0.5f, sinf(theta) / 2 + 0.5f) };
            Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);
        }

        // 放入索引
        for (UINT i = 0; i < slices; ++i)
        {
            meshData.indexVec[iIndex++] = i;
            meshData.indexVec[iIndex++] = slices + (i + 1) % slices;
            meshData.indexVec[iIndex++] = slices + i % slices;
        }

        return meshData;
    }

    // ----------------------------------------------------------
    // Create2DShow — 创建 2D 屏幕空间四边形（使用 XMFLOAT2 版）
    // 参数：
    //   center : NDC 中心坐标
    //   scale  : NDC 半尺寸
    //   color  : 顶点颜色
    // 说明：
    //   委托给下面的 float 重载版本。
    // ----------------------------------------------------------
    template<class VertexType, class IndexType>
    inline MeshData<VertexType, IndexType> Create2DShow(const DirectX::XMFLOAT2& center, const DirectX::XMFLOAT2 & scale, const DirectX::XMFLOAT4 & color)
    {
        return Create2DShow<VertexType, IndexType>(center.x, center.y, scale.x, scale.y, color);
    }

    // ----------------------------------------------------------
    // Create2DShow — 创建 2D 屏幕空间四边形（float 参数版）
    // 参数：
    //   centerX, centerY : NDC 中心坐标（范围 [-1, 1]）
    //   scaleX, scaleY   : NDC 半尺寸
    //   color            : 顶点颜色
    // 说明：
    //   在 NDC 空间中生成一个矩形四边形，用于 2D 全屏渲染或后处理。
    //   顶点按逆时针顺序排列，纹理坐标覆盖整个 [0,1] 区间。
    // ----------------------------------------------------------
    template<class VertexType, class IndexType>
    inline MeshData<VertexType, IndexType> Create2DShow(float centerX, float centerY, float scaleX, float scaleY, const DirectX::XMFLOAT4 & color)
    {
        using namespace DirectX;

        MeshData<VertexType, IndexType> meshData;
        meshData.vertexVec.resize(4);

        Internal::VertexData vertexData;
        UINT vIndex = 0;

        vertexData = { XMFLOAT3(centerX - scaleX, centerY - scaleY, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),
            XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(0.0f, 1.0f) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        vertexData = { XMFLOAT3(centerX - scaleX, centerY + scaleY, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),
            XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(0.0f, 0.0f) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        vertexData = { XMFLOAT3(centerX + scaleX, centerY + scaleY, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),
            XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(1.0f, 0.0f) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        vertexData = { XMFLOAT3(centerX + scaleX, centerY - scaleY, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),
            XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(1.0f, 1.0f) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        meshData.indexVec = { 0, 1, 2, 2, 3, 0 };
        return meshData;
    }

    // ----------------------------------------------------------
    // CreatePlane — 创建平面网格（XMFLOAT2 版）
    // 参数：
    //   planeSize   : 平面尺寸 (width, depth)
    //   maxTexCoord : 最大纹理坐标 (u, v)
    //   color       : 顶点颜色
    // 说明：
    //   委托给下面的 float 重载版本。
    // ----------------------------------------------------------
    template<class VertexType, class IndexType>
    inline MeshData<VertexType, IndexType> CreatePlane(const DirectX::XMFLOAT2 & planeSize,
        const DirectX::XMFLOAT2 & maxTexCoord, const DirectX::XMFLOAT4 & color)
    {
        return CreatePlane<VertexType, IndexType>(planeSize.x, planeSize.y, maxTexCoord.x, maxTexCoord.y, color);
    }

    // ----------------------------------------------------------
    // CreatePlane — 创建平面网格（float 参数版）
    // 参数：
    //   width  : X 轴方向宽度
    //   depth  : Z 轴方向深度
    //   texU   : 纹理 U 方向最大坐标
    //   texV   : 纹理 V 方向最大坐标
    //   color  : 顶点颜色
    // 说明：
    //   在 XZ 平面上（Y=0）生成长方形网格，法线朝上 (+Y)，
    //   由 4 个顶点和 2 个三角形构成。
    // ----------------------------------------------------------
    template<class VertexType, class IndexType>
    inline MeshData<VertexType, IndexType> CreatePlane(float width, float depth, float texU, float texV, const DirectX::XMFLOAT4 & color)
    {
        using namespace DirectX;

        MeshData<VertexType, IndexType> meshData;
        meshData.vertexVec.resize(4);

        Internal::VertexData vertexData;
        UINT vIndex = 0;

        vertexData = { XMFLOAT3(-width / 2, 0.0f, -depth / 2), XMFLOAT3(0.0f, 1.0f, 0.0f),
            XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(0.0f, texV) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        vertexData = { XMFLOAT3(-width / 2, 0.0f, depth / 2), XMFLOAT3(0.0f, 1.0f, 0.0f),
            XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(0.0f, 0.0f) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        vertexData = { XMFLOAT3(width / 2, 0.0f, depth / 2), XMFLOAT3(0.0f, 1.0f, 0.0f),
            XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(texU, 0.0f) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        vertexData = { XMFLOAT3(width / 2, 0.0f, -depth / 2), XMFLOAT3(0.0f, 1.0f, 0.0f),
            XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), color, XMFLOAT2(texU, texV) };
        Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);

        meshData.indexVec = { 0, 1, 2, 2, 3, 0 };
        return meshData;
    }

    // ----------------------------------------------------------
    // CreateTerrain — 创建地形网格（XMFLOAT2/XMUINT2 版）
    // 参数：
    //   terrainSize  : 地形尺寸 (width, depth)
    //   slices       : 分割数 (slicesX, slicesZ)
    //   maxTexCoord  : 最大纹理坐标
    //   heightFunc   : 高度函数 f(x, z) -> 高度 y
    //   normalFunc   : 法线函数 f(x, z) -> 法线向量
    //   colorFunc    : 颜色函数 f(x, z) -> 顶点颜色
    // 说明：
    //   委托给下面的 float 重载版本。
    // ----------------------------------------------------------
    template<class VertexType, class IndexType>
    MeshData<VertexType, IndexType> CreateTerrain(const DirectX::XMFLOAT2& terrainSize, const DirectX::XMUINT2& slices,
        const DirectX::XMFLOAT2& maxTexCoord, const std::function<float(float, float)>& heightFunc,
        const std::function<DirectX::XMFLOAT3(float, float)>& normalFunc, 
        const std::function<DirectX::XMFLOAT4(float, float)>& colorFunc)
    {
        return CreateTerrain<VertexType, IndexType>(terrainSize.x, terrainSize.y, slices.x, slices.y,
            maxTexCoord.x, maxTexCoord.y, heightFunc, normalFunc, colorFunc);
    }

    // ----------------------------------------------------------
    // CreateTerrain — 创建地形网格（float 参数版）
    // 参数：
    //   width      : X 轴方向宽度
    //   depth      : Z 轴方向深度
    //   slicesX    : X 方向分割数
    //   slicesZ    : Z 方向分割数
    //   texU, texV : 纹理坐标缩放
    //   heightFunc : 高度函数 f(x, z) -> 高度 y
    //   normalFunc : 法线函数 f(x, z) -> 法线向量
    //   colorFunc  : 颜色函数 f(x, z) -> 顶点颜色
    // 说明：
    //   在 XZ 平面上生成规则网格，然后根据 heightFunc 调整每个顶点的 Y 值。
    //   索引按两个三角形构成一个四边形网格的方式排列。
    //   同时计算每点的法线（由 normalFunc 提供）和切线（由法线推导）。
    // ----------------------------------------------------------
    template<class VertexType, class IndexType>
    MeshData<VertexType, IndexType> CreateTerrain(float width, float depth, UINT slicesX, UINT slicesZ,
        float texU, float texV, const std::function<float(float, float)>& heightFunc,
        const std::function<DirectX::XMFLOAT3(float, float)>& normalFunc,
        const std::function<DirectX::XMFLOAT4(float, float)>& colorFunc)
    {
        using namespace DirectX;

        MeshData<VertexType, IndexType> meshData;
        UINT vertexCount = (slicesX + 1) * (slicesZ + 1);
        UINT indexCount = 6 * slicesX * slicesZ;
        meshData.vertexVec.resize(vertexCount);
        meshData.indexVec.resize(indexCount);

        Internal::VertexData vertexData;
        UINT vIndex = 0;
        UINT iIndex = 0;

        float sliceWidth = width / slicesX;
        float sliceDepth = depth / slicesZ;
        float leftBottomX = -width / 2;
        float leftBottomZ = -depth / 2;
        float posX, posZ;
        float sliceTexWidth = texU / slicesX;
        float sliceTexDepth = texV / slicesZ;

        XMFLOAT3 normal;
        XMFLOAT4 tangent;
        // 创建网格顶点
        //  __ __
        // | /| /|
        // |/_|/_|
        // | /| /| 
        // |/_|/_|
        for (UINT z = 0; z <= slicesZ; ++z)
        {
            posZ = leftBottomZ + z * sliceDepth;
            for (UINT x = 0; x <= slicesX; ++x)
            {
                posX = leftBottomX + x * sliceWidth;
                // 计算法向量并归一化
                normal = normalFunc(posX, posZ);
                XMStoreFloat3(&normal, XMVector3Normalize(XMLoadFloat3(&normal)));
                // 计算法平面与z=posZ平面构成的直线单位切向量，维持w分量为1.0f
                XMStoreFloat4(&tangent, XMVector3Normalize(XMVectorSet(normal.y, -normal.x, 0.0f, 0.0f)) + g_XMIdentityR3);

                vertexData = { XMFLOAT3(posX, heightFunc(posX, posZ), posZ),
                    normal, tangent, colorFunc(posX, posZ), XMFLOAT2(x * sliceTexWidth, texV - z * sliceTexDepth) };
                Internal::InsertVertexElement(meshData.vertexVec[vIndex++], vertexData);
            }
        }
        // 放入索引
        for (UINT i = 0; i < slicesZ; ++i)
        {
            for (UINT j = 0; j < slicesX; ++j)
            {
                meshData.indexVec[iIndex++] = i * (slicesX + 1) + j;
                meshData.indexVec[iIndex++] = (i + 1) * (slicesX + 1) + j;
                meshData.indexVec[iIndex++] = (i + 1) * (slicesX + 1) + j + 1;

                meshData.indexVec[iIndex++] = (i + 1) * (slicesX + 1) + j + 1;
                meshData.indexVec[iIndex++] = i * (slicesX + 1) + j + 1;
                meshData.indexVec[iIndex++] = i * (slicesX + 1) + j;
            }
        }

        return meshData;
    }


}


#endif