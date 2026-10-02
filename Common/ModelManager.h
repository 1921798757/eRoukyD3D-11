//***************************************************************************************
// ModelManager.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 模型与模型管理器
// Model and model manager.
//
// 【文件作用】
//   定义了两个东西：
//     1. Model        —— 一个完整的 3D 模型 = 多个子网格(MeshData) + 多个材质(Material)
//                        + 整体包围盒。它是"数据"的载体。
//     2. ModelManager —— 单例管理器，负责创建、缓存、按名字查找 Model。
//                        它是"工厂 + 资源池"。
//
// 【三者的关系】
//     ModelManager  ──持有──>  多个 Model
//     Model         ──持有──>  vector<MeshData>(几何) + vector<Material>(外观)
//     MeshData      ──通过 m_MaterialIndex 关联──>  Material
//
//   渲染时（见 GameObject::Draw）：遍历每个 MeshData，
//   先用 m_MaterialIndex 找到对应 Material 设置给 Effect，
//   再把 MeshData 的顶点/索引缓冲绑定到管线绘制。
//***************************************************************************************

#pragma once

#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "WinMin.h"
#include "Geometry.h"    // GeometryData —— 程序化生成的几何数据（盒子/球/圆柱等）
#include "Material.h"
#include "MeshData.h"
#include <d3d11_1.h>
#include <wrl/client.h>

// 一个完整的 3D 模型
// 注意：禁止拷贝（内含大量 GPU 资源），但允许移动。
struct Model
{
    Model() = default;
    ~Model() = default;
    Model(Model&) = delete;                 // 禁止拷贝构造
    Model& operator=(const Model&) = delete; // 禁止拷贝赋值
    Model(Model&&) = default;               // 允许移动构造（ComPtr/容器都可安全移动）
    Model& operator=(Model&&) = default;    // 允许移动赋值

    std::vector<Material> materials;    // 该模型的全部材质列表
    std::vector<MeshData> meshdatas;    // 该模型的全部子网格列表
    DirectX::BoundingBox boundingbox;   // 整个模型的包围盒（所有子网格包围盒的并集）

    // 从模型文件（FBX/OBJ 等，assimp 支持的格式）加载并创建 GPU 资源
    // 静态函数：把结果写入传入的 model 引用（而不是返回新对象，便于管理器原地构造）
    static void CreateFromFile(Model& model, ID3D11Device* device, std::string_view filename);

    // 用程序化几何数据（GeometryData）创建模型，附带一个默认材质
    // isDynamic = true 时顶点缓冲为 D3D11_USAGE_DYNAMIC，可每帧 Map 更新
    static void CreateFromGeometry(Model& model, ID3D11Device* device, const GeometryData& data, bool isDynamic = false);
    
    // 给所有 GPU 缓冲设置调试名称，便于在 PIX/RenderDoc 中识别
    void SetDebugObjectName(std::string_view name);
};


// 模型管理器：单例，负责创建与缓存所有 Model
class ModelManager
{
public:
    ModelManager();
    ~ModelManager();
    ModelManager(ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;
    ModelManager(ModelManager&&) = default;
    ModelManager& operator=(ModelManager&&) = default;

    // 获取全局唯一实例（必须先构造过一个，否则抛异常）
    static ModelManager& Get();

    // 初始化：记住 D3D 设备（创建缓冲需要它）
    void Init(ID3D11Device* device);

    // 从文件创建模型。名字缺省时用文件路径作为名字。
    // 返回的指针指向管理器内部存储，调用方不要 delete。
    Model* CreateFromFile(std::string_view filename);
    Model* CreateFromFile(std::string_view name, std::string_view filename);

    // 用程序化几何创建模型（如地面、盒子等基础图形）
    Model* CreateFromGeometry(std::string_view name, const GeometryData& data, bool isDynamic = false);

    // 按名字查找模型，找不到返回 nullptr
    const Model* GetModel(std::string_view name) const;
    Model* GetModel(std::string_view name);
private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;          // D3D 设备，用于创建顶点/索引缓冲
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pDeviceContext;
    std::unordered_map<size_t, Model> m_Models;              // 名字哈希(XID) → Model 的缓存池
};


#endif // MODELMANAGER_H
