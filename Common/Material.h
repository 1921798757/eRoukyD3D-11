//***************************************************************************************
// Material.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 存放材质与属性
// Material and property storage.
//
// 【文件作用】
//   Material 是一个"属性包"——它不写死固定的材质字段（如 diffuse、specular），
//   而是用一张"名字 → 值"的哈希表来存放任意属性。
//   例如："$DiffuseColor"(XMFLOAT4)、"$Opacity"(float)、"$Diffuse"(纹理路径字符串)。
//
// 【为什么这样设计？】
//   不同模型格式（FBX/OBJ/glTF...）携带的材质属性种类不一，
//   不同着色器（Phong、PBR...）需要的属性也不同。
//   用"键值对 + std::variant"的方式，一个 Material 结构就能兼容所有情况：
//     - 加载时（ModelManager::CreateFromFile）把 assimp 材质里有的属性都塞进来；
//     - 渲染时（BasicEffect::SetMaterial）按需 Get/TryGet 自己关心的属性。
//
// 【属性名的约定】
//   以 "$" 开头的名字是本项目约定的标准属性键，例如：
//     "$AmbientColor" / "$DiffuseColor" / "$SpecularColor" / "$EmissiveColor"
//     "$SpecularFactor" / "$Opacity"
//     "$Diffuse"(漫反射纹理路径) / "$Normal"(法线贴图路径) / "$Albedo"(PBR 基础色)
//***************************************************************************************


#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include <string_view>
#include <unordered_map>
#include "XUtil.h"      // XID / StringToID —— 字符串哈希
#include "Property.h"   // Property = std::variant<...> 属性值的类型集合

class Material
{
public:
    Material() = default;

    // 清空所有属性
    void Clear()
    {
        m_Properties.clear();
    }

    // ------------------------------ Set ------------------------------
    // 设置/覆盖一个属性。
    //   name  —— 属性名（如 "$DiffuseColor"），内部先哈希成 XID 再作键
    //   value —— 属性值，类型必须是 Property(variant) 中列出的类型之一
    // static_assert 在编译期检查 T 是否属于 variant 的候选类型，
    // 防止塞进一个取出时根本无法表示的类型。
    template<class T>
    void Set(std::string_view name, const T& value)
    {
        static_assert(IsVariantMember<T, Property>::value, "Type T isn't one of the Property types!");
        m_Properties[StringToID(name)] = value;
    }

    // ------------------------------ Get ------------------------------
    // 按名字取出属性值的只读引用。
    // 注意：属性不存在或类型不匹配时，std::get 会抛异常/未定义行为，
    // 因此只应在"确定存在"时使用；不确定请用下面的 TryGet / Has。
    template<class T>
    const T& Get(std::string_view name) const
    {
        auto it = m_Properties.find(StringToID(name));
        return std::get<T>(it->second);
    }

    // 非 const 重载：借助 const 版本实现，避免重复代码。
    // 技巧：先把 this 转成 const Material* 调用 const 版，再 const_cast 回可变引用。
    template<class T>
    T& Get(std::string_view name)
    {
        return const_cast<T&>(static_cast<const Material*>(this)->Get<T>(name));
    }

    // ------------------------------ Has ------------------------------
    // 判断"是否存在该名字的属性，且其值的类型恰好是 T"。
    // std::holds_alternative<T> 检查 variant 当前实际保存的类型。
    // 典型用法：渲染前先 Has<float>("$SpecularFactor")，没有就用默认值。
    template<class T>
    bool Has(std::string_view name) const
    {
        auto it = m_Properties.find(StringToID(name));
        if (it == m_Properties.end() || !std::holds_alternative<T>(it->second))
            return false;
        return true;
    }

    // ------------------------------ TryGet ------------------------------
    // 安全取值：找到且类型匹配则返回指向值的指针，否则返回 nullptr。
    // 比 Get 更安全，适合"可选属性"的场景。
    // 例如 BasicEffect 中：auto pStr = material.TryGet<std::string>("$Diffuse");
    template<class T>
    const T* TryGet(std::string_view name) const
    {
        auto it = m_Properties.find(StringToID(name));
        if (it != m_Properties.end())
            return &std::get<T>(it->second);
        else
            return nullptr;
    }

    // 非 const 重载，同样复用 const 版本
    template<class T>
    T* TryGet(std::string_view name)
    {
        return const_cast<T*>(static_cast<const Material*>(this)->TryGet<T>(name));
    }

    // 只判断"该名字的属性是否存在"（不关心值的类型）
    bool HasProperty(std::string_view name) const
    {
        return m_Properties.find(StringToID(name)) != m_Properties.end();
    }

private:

    // 核心存储：属性名哈希(XID) → 属性值(Property/variant)
    // 用哈希值作键而不是字符串，比较和查找都是整数运算，更快。
    std::unordered_map<XID, Property> m_Properties;
};


#endif
