// ============================================================================
// ModelManager.cpp — 模型加载与创建的实现
// ============================================================================
// 本文件是 MeshData / Material 的"生产车间"：
//   1. Model::CreateFromFile     —— 用 assimp 读取模型文件，把几何数据转成
//                                   一个个 GPU 顶点/索引缓冲填入 MeshData，
//                                   把材质属性和纹理路径填入 Material。
//   2. Model::CreateFromGeometry —— 用程序化几何数据创建模型（附默认材质）。
//   3. ModelManager              —— 单例，按名字缓存和管理所有 Model。
// ============================================================================

#include "XUtil.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "ImGuiLog.h"

#include <filesystem>

#include <assimp/Importer.hpp>   // assimp 模型导入器
#include <assimp/postprocess.h>  // assimp 后处理标志
#include <assimp/scene.h>        // assimp 场景/网格/材质数据结构

using namespace DirectX;

// ----------------------------------------------------------------------------
// Model::CreateFromFile —— 从文件加载模型
// 流程：assimp 读文件 → 遍历每个 aiMesh 创建顶点/索引缓冲 → 遍历每个
//       aiMaterial 提取颜色和纹理 → 全部存入 model 的 meshdatas / materials
// ----------------------------------------------------------------------------
void Model::CreateFromFile(Model& model, ID3D11Device* device, std::string_view filename)
{
    using namespace Assimp;
    namespace fs = std::filesystem;
    
    // 清空旧数据，重新加载
    model.materials.clear();
    model.meshdatas.clear();
    model.boundingbox = BoundingBox();

    Importer importer;
    // 配置：在后处理阶段移除"点"和"线"图元，只保留三角形
    // （本渲染器只画三角形，点/线数据没有意义）
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    // 读取模型文件并执行一系列后处理步骤
    auto pAssimpScene = importer.ReadFile(filename.data(), 
        aiProcess_ConvertToLeftHanded |     // 转为左手系（DirectX 用左手系，建模软件多为右手系）
        aiProcess_GenBoundingBoxes |        // 让 assimp 生成包围盒
        aiProcess_Triangulate |             // 把四边形/多边形面拆成三角形
        aiProcess_ImproveCacheLocality |    // 重排索引顺序，提高 GPU 顶点缓存命中率
        aiProcess_SortByPType);             // 按图元类型分组，配合上面的配置移除非三角形图元

    // 加载成功且场景完整、至少有一个网格
    if (pAssimpScene && !(pAssimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && pAssimpScene->HasMeshes())
    {
        // 按 assimp 场景中的数量预分配子网格和材质容器
        model.meshdatas.resize(pAssimpScene->mNumMeshes);
        model.materials.resize(pAssimpScene->mNumMaterials);

        // ==================== 第一阶段：转换每个网格的几何数据 ====================
        for (uint32_t i = 0; i < pAssimpScene->mNumMeshes; ++i)
        {
            auto& mesh = model.meshdatas[i];      // 本项目侧的目标网格

            auto pAiMesh = pAssimpScene->mMeshes[i];   // assimp 侧的源网格
            uint32_t numVertices = pAiMesh->mNumVertices;

            // 缓冲描述：初始为顶点缓冲用途，字节数稍后按属性填写
            CD3D11_BUFFER_DESC bufferDesc(0, D3D11_BIND_VERTEX_BUFFER);
            // 初始数据：pSysMem 指向内存中的原始数据，GPU 创建缓冲时拷贝它
            D3D11_SUBRESOURCE_DATA initData{ nullptr, 0, 0 };

            // ---------- 顶点位置（必有） ----------
            if (pAiMesh->mNumVertices > 0)
            {
                initData.pSysMem = pAiMesh->mVertices;
                bufferDesc.ByteWidth = numVertices * sizeof(XMFLOAT3);
                // 创建 GPU 顶点缓冲。GetAddressOf() 取出 ComPtr 内部指针的地址用于输出
                device->CreateBuffer(&bufferDesc, &initData, mesh.m_pVertices.GetAddressOf());

                // 用所有顶点位置计算该子网格的包围盒（局部空间）
                BoundingBox::CreateFromPoints(mesh.m_BoundingBox, numVertices,
                    (const XMFLOAT3*)pAiMesh->mVertices, sizeof(XMFLOAT3));
                // 同时合并出整个模型的大包围盒
                if (i == 0)
                    model.boundingbox = mesh.m_BoundingBox;
                else
                    model.boundingbox.CreateMerged(model.boundingbox, model.boundingbox, mesh.m_BoundingBox);
            }

            // ---------- 法线（用于光照） ----------
            if (pAiMesh->HasNormals())
            {
                initData.pSysMem = pAiMesh->mNormals;
                bufferDesc.ByteWidth = numVertices * sizeof(XMFLOAT3);
                device->CreateBuffer(&bufferDesc, &initData, mesh.m_pNormals.GetAddressOf());
            }

            // ---------- 切线与副切线（用于法线贴图的 TBN 空间） ----------
            if (pAiMesh->HasTangentsAndBitangents())
            {
                // assimp 的切线是 3 分量(float3)，这里扩成 XMFLOAT4 存入缓冲，
                // 第 4 分量 w 初始化为 1（供切线手性判断等用途）。
                // 所以需要一个中转数组，不能直接拿 assimp 的指针创建缓冲。
                std::vector<XMFLOAT4> tangents(numVertices, XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
                for (uint32_t i = 0; i < pAiMesh->mNumVertices; ++i)
                {
                    // 每个顶点拷贝 3 个 float（xyz），w 保持为 1
                    memcpy_s(&tangents[i], sizeof(XMFLOAT3),
                        pAiMesh->mTangents + i, sizeof(XMFLOAT3));
                }

                initData.pSysMem = tangents.data();
                bufferDesc.ByteWidth = pAiMesh->mNumVertices * sizeof(XMFLOAT4);
                device->CreateBuffer(&bufferDesc, &initData, mesh.m_pTangents.GetAddressOf());

                // 副切线用同样的方式处理（复用同一个中转数组）
                for (uint32_t i = 0; i < pAiMesh->mNumVertices; ++i)
                {
                    memcpy_s(&tangents[i], sizeof(XMFLOAT3),
                        pAiMesh->mBitangents + i, sizeof(XMFLOAT3));
                }
                device->CreateBuffer(&bufferDesc, &initData, mesh.m_pBitangents.GetAddressOf());
            }

            // ---------- 纹理坐标（最多 8 套 UV） ----------
            // 从高往低探测实际存在几套 UV
            uint32_t numUVs = 8;
            while (numUVs && !pAiMesh->HasTextureCoords(numUVs - 1))
                numUVs--;

            if (numUVs > 0)
            {
                mesh.m_pTexcoordArrays.resize(numUVs);
                for (uint32_t i = 0; i < numUVs; ++i)
                {
                    // assimp 的 UV 是 3 分量，这里只取前 2 分量存成 XMFLOAT2，
                    // 因此同样需要中转数组逐顶点拷贝
                    std::vector<XMFLOAT2> uvs(numVertices);
                    for (uint32_t j = 0; j < numVertices; ++j)
                    {
                        memcpy_s(&uvs[j], sizeof(XMFLOAT2),
                            pAiMesh->mTextureCoords[i] + j, sizeof(XMFLOAT2));
                    }
                    initData.pSysMem = uvs.data();
                    bufferDesc.ByteWidth = numVertices * sizeof(XMFLOAT2);
                    device->CreateBuffer(&bufferDesc, &initData, mesh.m_pTexcoordArrays[i].GetAddressOf());
                }
            }

            // ---------- 索引缓冲 ----------
            // 经过 Triangulate 后每个面都是三角形，索引总数 = 面数 * 3
            uint32_t numFaces = pAiMesh->mNumFaces;
            uint32_t numIndices = numFaces * 3;
            if (numFaces > 0)
            {
                mesh.m_IndexCount = numIndices;
                if (numIndices < 65535)
                {
                    // 顶点数少时用 16 位索引，省显存、带宽减半
                    std::vector<uint16_t> indices(numIndices);
                    for (size_t i = 0; i < numFaces; ++i)
                    {
                        // 把每个三角形面的 3 个顶点索引依次写入
                        indices[i * 3] = static_cast<uint16_t>(pAiMesh->mFaces[i].mIndices[0]);
                        indices[i * 3 + 1] = static_cast<uint16_t>(pAiMesh->mFaces[i].mIndices[1]);
                        indices[i * 3 + 2] = static_cast<uint16_t>(pAiMesh->mFaces[i].mIndices[2]);
                    }
                    bufferDesc = CD3D11_BUFFER_DESC(numIndices * sizeof(uint16_t), D3D11_BIND_INDEX_BUFFER);
                    initData.pSysMem = indices.data();
                    device->CreateBuffer(&bufferDesc, &initData, mesh.m_pIndices.GetAddressOf());
                }
                else
                {
                    // 大网格用 32 位索引（16 位最多只能索引 65535 个顶点）
                    std::vector<uint32_t> indices(numIndices);
                    for (size_t i = 0; i < numFaces; ++i)
                    {
                        // 每个面 3 个 uint32 索引，整块拷贝
                        memcpy_s(indices.data() + i * 3, sizeof(uint32_t) * 3,
                            pAiMesh->mFaces[i].mIndices, sizeof(uint32_t) * 3);
                    }
                    bufferDesc = CD3D11_BUFFER_DESC(numIndices * sizeof(uint32_t), D3D11_BIND_INDEX_BUFFER);
                    initData.pSysMem = indices.data();
                    device->CreateBuffer(&bufferDesc, &initData, mesh.m_pIndices.GetAddressOf());
                }
            }

            // 记录该子网格使用哪个材质（索引指向 model.materials）
            mesh.m_MaterialIndex = pAiMesh->mMaterialIndex;
        }


        // ==================== 第二阶段：转换每个材质 ====================
        for (uint32_t i = 0; i < pAssimpScene->mNumMaterials; ++i)
        {
            auto& material = model.materials[i];

            auto pAiMaterial = pAssimpScene->mMaterials[i];
            XMFLOAT4 vec{};       // 接收颜色类属性（RGB + 可能的 alpha）
            float value{};        // 接收标量属性
            uint32_t boolean{};
            uint32_t num = 3;     // 告诉 assimp 我们最多接收 3 个 float（RGB）

            // ---------- 提取颜色/标量属性 ----------
            // assimp 的 Get 返回 aiReturn_SUCCESS 表示该属性存在，才写入 Material。
            // 属性名以 "$" 开头是本项目约定的标准键名。
            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_AMBIENT, (float*)&vec, &num))
                material.Set("$AmbientColor", vec);      // 环境光颜色
            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, (float*)&vec, &num))
                material.Set("$DiffuseColor", vec);      // 漫反射颜色
            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_SPECULAR, (float*)&vec, &num))
                material.Set("$SpecularColor", vec);     // 高光颜色
            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_SPECULAR_FACTOR, value))
                material.Set("$SpecularFactor", value);  // 高光强度（Phong 指数）
            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, (float*)&vec, &num))
                material.Set("$EmissiveColor", vec);     // 自发光颜色
            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_OPACITY, value))
                material.Set("$Opacity", value);         // 不透明度
            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_TRANSPARENT, (float*)&vec, &num))
                material.Set("$TransparentColor", vec);  // 透明颜色
            if (aiReturn_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_REFLECTIVE, (float*)&vec, &num))
                material.Set("$ReflectiveColor", vec);   // 反射颜色
            
            aiString aiPath;
            fs::path texFilename;
            std::string texName;

            // ---------- 提取纹理 ----------
            // 局部 lambda：尝试获取指定类型的纹理并注册到 TextureManager。
            //   type         —— assimp 纹理类型（漫反射/法线/金属度...）
            //   propertyName —— 存入 Material 的属性键名（如 "$Diffuse"）
            //   genMips      —— 是否生成 mipmap
            //   forceSRGB    —— 是否按 sRGB 格式加载（颜色贴图需要，数据贴图不需要）
            // 成功后，Material 里存的是"纹理的名字/路径"（字符串），
            // 渲染时再用这个名字去 TextureManager 取真正的纹理对象。
            auto TryCreateTexture = [&](aiTextureType type, std::string_view propertyName, bool genMips = false, bool forceSRGB = false) {
                // 该材质没有这种纹理就跳过
                if (!pAiMaterial->GetTextureCount(type))
                    return;

                // 取第 0 张该类型纹理的路径
                pAiMaterial->GetTexture(type, 0, &aiPath);

                // 情况一：纹理数据内嵌在模型文件里（路径形如 "*0"，表示场景纹理数组的下标）
                if (aiPath.data[0] == '*')
                {
                    // 用 "模型文件名 + *下标" 作为纹理的唯一名字
                    texName = filename;
                    texName += aiPath.C_Str();
                    // 解析下标，从场景的纹理数组中取出内存数据
                    char* pEndStr = nullptr;
                    aiTexture* pTex = pAssimpScene->mTextures[strtol(aiPath.data + 1, &pEndStr, 10)];
                    // 从内存创建纹理。mHeight 为 0 表示数据是压缩格式（如 PNG），
                    // 此时 mWidth 存的是字节数；否则是未压缩的宽x高像素数
                    TextureManager::Get().CreateFromMemory(texName, pTex->pcData, pTex->mHeight ? pTex->mWidth * pTex->mHeight : pTex->mWidth, genMips, forceSRGB);
                    material.Set(propertyName, std::string(texName));
                }
                // 情况二：纹理是外部文件，路径是相对模型文件的文件名
                else
                {
                    // 拼出纹理文件的完整路径：模型所在目录 / 相对路径
                    texFilename = filename;
                    texFilename = texFilename.parent_path() / aiPath.C_Str();
                    TextureManager::Get().CreateFromFile(texFilename.string(), genMips, forceSRGB);
                    material.Set(propertyName, texFilename.string());
                }
            };

            // 依次尝试提取各种纹理。
            // 颜色类贴图（漫反射/基础色）开 mipmap 并强制 sRGB；
            // 数据类贴图（法线/金属度/粗糙度）保持线性空间。
            TryCreateTexture(aiTextureType_DIFFUSE, "$Diffuse", true, true);          // 漫反射贴图
            TryCreateTexture(aiTextureType_NORMALS, "$Normal");                       // 法线贴图
            TryCreateTexture(aiTextureType_BASE_COLOR, "$Albedo", true, true);        // PBR 基础色贴图
            TryCreateTexture(aiTextureType_NORMAL_CAMERA, "$NormalCamera");           // 相机空间法线贴图
            TryCreateTexture(aiTextureType_METALNESS, "$Metalness");                  // 金属度
            TryCreateTexture(aiTextureType_DIFFUSE_ROUGHNESS, "$Roughness");          // 粗糙度
            TryCreateTexture(aiTextureType_AMBIENT_OCCLUSION, "$AmbientOcclusion");   // 环境光遮蔽(AO)
        }
    }
    else
    {
        // 加载失败：输出警告到 ImGui 日志窗口（若有），否则输出到调试器
        std::string warning = "[Warning]: ModelManager::CreateFromFile, failed to load \"";
        warning += filename;
        warning += "\"\n";

        if (ImGuiLog::HasInstance())
        {
            ImGuiLog::Get().AddLog(warning.c_str());
        }
        else
        {
            OutputDebugStringA(warning.c_str());
        }
    }
}

// ----------------------------------------------------------------------------
// Model::CreateFromGeometry —— 用程序化几何数据创建模型
// 用于盒子、球、圆柱、地面等代码生成的图形，附带一个默认白色材质。
// ----------------------------------------------------------------------------
void Model::CreateFromGeometry(Model& model, ID3D11Device* device, const GeometryData& data, bool isDynamic)
{
    // ---------- 默认材质：一套经典 Phong 属性 ----------
    model.materials = { Material{} };
    model.materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));  // 暗灰环境光
    model.materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));  // 浅灰漫反射
    model.materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f)); // 弱高光
    model.materials[0].Set<float>("$SpecularFactor", 10.0f);                              // 高光指数
    model.materials[0].Set<float>("$Opacity", 1.0f);                                      // 不透明

    // ---------- 单个子网格 ----------
    model.meshdatas = { MeshData{} };
    model.meshdatas[0].m_pTexcoordArrays.resize(1);   // 程序化几何只有一套 UV
    model.meshdatas[0].m_VertexCount = (uint32_t)data.vertices.size();
    // 索引数：优先用 16 位索引，否则用 32 位
    model.meshdatas[0].m_IndexCount = (uint32_t)(!data.indices16.empty() ? data.indices16.size() : data.indices32.size());
    model.meshdatas[0].m_MaterialIndex = 0;           // 使用上面那个默认材质

    // 顶点缓冲描述：
    //   isDynamic = true  → D3D11_USAGE_DYNAMIC + CPU 可写，允许每帧 Map 更新顶点
    //   isDynamic = false → D3D11_USAGE_DEFAULT，创建后不可改，性能更好
    CD3D11_BUFFER_DESC bufferDesc(0,
        D3D11_BIND_VERTEX_BUFFER,
        isDynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
        isDynamic ? D3D11_CPU_ACCESS_WRITE : 0);
    D3D11_SUBRESOURCE_DATA initData{ nullptr, 0, 0 };

    // ---------- 位置 ----------
    initData.pSysMem = data.vertices.data();
    bufferDesc.ByteWidth = (uint32_t)data.vertices.size() * sizeof(XMFLOAT3);
    device->CreateBuffer(&bufferDesc, &initData, model.meshdatas[0].m_pVertices.GetAddressOf());

    // ---------- 法线（有则创建） ----------
    if (!data.normals.empty())
    {
        initData.pSysMem = data.normals.data();
        bufferDesc.ByteWidth = (uint32_t)data.normals.size() * sizeof(XMFLOAT3);
        device->CreateBuffer(&bufferDesc, &initData, model.meshdatas[0].m_pNormals.GetAddressOf());
    }

    // ---------- 纹理坐标（有则创建） ----------
    if (!data.texcoords.empty())
    {
        initData.pSysMem = data.texcoords.data();
        bufferDesc.ByteWidth = (uint32_t)data.texcoords.size() * sizeof(XMFLOAT2);
        device->CreateBuffer(&bufferDesc, &initData, model.meshdatas[0].m_pTexcoordArrays[0].GetAddressOf());
    }

    // ---------- 切线（有则创建） ----------
    if (!data.tangents.empty())
    {
        initData.pSysMem = data.tangents.data();
        bufferDesc.ByteWidth = (uint32_t)data.tangents.size() * sizeof(XMFLOAT4);
        device->CreateBuffer(&bufferDesc, &initData, model.meshdatas[0].m_pTangents.GetAddressOf());
    }

    // ---------- 索引缓冲（索引总是 DEFAULT 用途，不需要动态更新） ----------
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.CPUAccessFlags = 0;
    if (!data.indices16.empty())
    {
        // 16 位索引
        initData.pSysMem = data.indices16.data();
        bufferDesc = CD3D11_BUFFER_DESC((uint16_t)data.indices16.size() * sizeof(uint16_t), D3D11_BIND_INDEX_BUFFER);
        device->CreateBuffer(&bufferDesc, &initData, model.meshdatas[0].m_pIndices.GetAddressOf());
    }
    else
    {
        // 32 位索引
        initData.pSysMem = data.indices32.data();
        bufferDesc = CD3D11_BUFFER_DESC((uint32_t)data.indices32.size() * sizeof(uint32_t), D3D11_BIND_INDEX_BUFFER);
        device->CreateBuffer(&bufferDesc, &initData, model.meshdatas[0].m_pIndices.GetAddressOf());
    }
}

// ----------------------------------------------------------------------------
// Model::SetDebugObjectName —— 给所有 GPU 缓冲起调试名字
// 仅在 Debug 且开启图形调试器命名时生效，方便在 PIX/RenderDoc 里识别缓冲。
// 命名格式：模型名[子网格序号].属性名，如 "player[0].vertices"
// ----------------------------------------------------------------------------
void Model::SetDebugObjectName(std::string_view name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    std::string baseStr = name.data();
    size_t sz = meshdatas.size();
    std::string str;
    str.reserve(100);
    for (size_t i = 0; i < sz; ++i)
    {
        baseStr = name.data();
        baseStr += "[" + std::to_string(i) + "].";
        // 逐个缓冲设置名字（空指针的跳过）
        if (meshdatas[i].m_pVertices)
            ::SetDebugObjectName(meshdatas[i].m_pVertices.Get(), baseStr + "vertices");
        if (meshdatas[i].m_pNormals)
            ::SetDebugObjectName(meshdatas[i].m_pNormals.Get(), baseStr + "normals");
        if (meshdatas[i].m_pTangents)
            ::SetDebugObjectName(meshdatas[i].m_pTangents.Get(), baseStr + "tangents");
        if (meshdatas[i].m_pBitangents)
            ::SetDebugObjectName(meshdatas[i].m_pBitangents.Get(), baseStr + "bitangents");
        if (meshdatas[i].m_pColors)
            ::SetDebugObjectName(meshdatas[i].m_pColors.Get(), baseStr + "colors");
        if (!meshdatas[i].m_pTexcoordArrays.empty())
        {
            // 多套 UV 依次编号：uv0, uv1, ...
            size_t texSz = meshdatas[i].m_pTexcoordArrays.size();
            for (size_t j = 0; j < texSz; ++j)
                ::SetDebugObjectName(meshdatas[i].m_pTexcoordArrays[j].Get(), baseStr + "uv" + std::to_string(j));
        }
        if (meshdatas[i].m_pIndices)
            ::SetDebugObjectName(meshdatas[i].m_pIndices.Get(), baseStr + "indices");
    }
#else
    // Release 模式下什么都不做，避免"未使用参数"警告
    UNREFERENCED_PARAMETER(name);
#endif
}

namespace
{
    // ModelManager单例指针（匿名命名空间，仅本文件可见）
    ModelManager* s_pInstance = nullptr;
}


// 构造时注册自己为唯一实例；已有实例则报错（保证单例）
ModelManager::ModelManager()
{
    if (s_pInstance)
        throw std::exception("ModelManager is a singleton!");
    s_pInstance = this;
}

ModelManager::~ModelManager()
{
}

// 获取全局实例。未构造过则抛异常
ModelManager& ModelManager::Get()
{
    if (!s_pInstance)
        throw std::exception("ModelManager needs an instance!");
    return *s_pInstance;
}

// 初始化：保存设备指针，并取出立即上下文备用
void ModelManager::Init(ID3D11Device* device)
{
    m_pDevice = device;
    m_pDevice->GetImmediateContext(m_pDeviceContext.ReleaseAndGetAddressOf());
}

// 从文件创建：名字缺省时用文件路径当名字
Model* ModelManager::CreateFromFile(std::string_view filename)
{
    return CreateFromFile(filename, filename);
}

// 从文件创建：名字 → 哈希 → 在缓存池中创建/覆盖对应 Model
// 注意：返回的指针指向 map 内部，map 再插入可能使迭代器失效，
// 但 unordered_map 的插入不会使已有元素的指针/引用失效，所以安全。
Model* ModelManager::CreateFromFile(std::string_view name, std::string_view filename)
{
    XID modelID = StringToID(name);
    auto& model = m_Models[modelID];
    Model::CreateFromFile(model, m_pDevice.Get(), filename);
    return &model;
}

// 从程序化几何创建，逻辑同上
Model* ModelManager::CreateFromGeometry(std::string_view name, const GeometryData& data, bool isDynamic)
{
    XID modelID = StringToID(name);
    auto& model = m_Models[modelID];
    Model::CreateFromGeometry(model, m_pDevice.Get(), data, isDynamic);

    return &model;
}

// 按名字查找模型（只读版本）
const Model* ModelManager::GetModel(std::string_view name) const
{
    XID nameID = StringToID(name);
    if (auto it = m_Models.find(nameID); it != m_Models.end())
        return &it->second;
    return nullptr;
}

// 按名字查找模型（可写版本）
Model* ModelManager::GetModel(std::string_view name)
{
    XID nameID = StringToID(name);
    if (m_Models.count(nameID))
        return &m_Models[nameID];
    return nullptr;
}
