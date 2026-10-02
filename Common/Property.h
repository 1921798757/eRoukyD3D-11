// ============================================================================
// Property.h — 材质属性值的类型定义
// ============================================================================
// 本文件定义了两样东西：
//   1. Property —— 一个 std::variant，列出"材质属性值"允许的所有类型。
//      Material 中存储的每一个属性值都是这个类型。
//   2. IsVariantMember —— 编译期工具，用于判断某个类型 T 是否属于
//      variant 的候选类型之一（供 Material::Set 的 static_assert 使用）。
//
// 为什么用 std::variant？
//   材质属性种类多样：颜色是 XMFLOAT4，粗糙度是 float，纹理路径是 string，
//   骨骼矩阵是 vector<XMFLOAT4X4>…… variant 让一个容器能安全地存放
//   这些不同类型，并在取值时做类型检查（类型不对会抛异常而不是读错内存）。
// ============================================================================

#pragma once

#ifndef PROPERTY_H
#define PROPERTY_H

#include <memory>
#include <variant>
#include <vector>
#include <string>
#include <DirectXMath.h>

// ------------------------------ IsVariantMember ------------------------------
// 编译期判断：类型 T 是否是 variant<ALL_V...> 的候选类型之一。
//
// 原理：
//   - 主模板只声明不定义（通用情况不存在，实例化即报错）；
//   - 偏特化版本匹配任何 std::variant<ALL_V...>；
//   - std::is_same<T, ALL_V>... 对每个候选类型做"是否相同"判断，
//     std::disjunction 相当于逻辑或：只要有一个相同，value 就为 true。
//
// 用法：IsVariantMember<float, Property>::value == true
//       IsVariantMember<double, Property>::value == false（double 不在列表里）
template<class T, class V>
struct IsVariantMember;

template<class T, class... ALL_V>
struct IsVariantMember<T, std::variant<ALL_V...>> : public std::disjunction<std::is_same<T, ALL_V>...> {};

// ------------------------------ Property ------------------------------
// 材质属性值的合法类型集合：
//   标量：    int, uint32_t, float
//   向量/矩阵：XMFLOAT2(UV), XMFLOAT3(方向), XMFLOAT4(颜色/带权向量),
//              XMFLOAT4X4(变换矩阵)
//   数组：    vector<float>(一组标量),
//              vector<XMFLOAT4>(一组颜色/骨骼权重),
//              vector<XMFLOAT4X4>(骨骼动画的矩阵调色板)
//   字符串：  std::string（典型用途：纹理文件路径）
//
// 注意：想新增属性类型，必须先把类型加进这个 variant，
// 否则 Material::Set 的 static_assert 会编译报错。
using Property = std::variant<
    int, uint32_t, float, DirectX::XMFLOAT2, DirectX::XMFLOAT3, DirectX::XMFLOAT4, DirectX::XMFLOAT4X4, 
    std::vector<float>, std::vector<DirectX::XMFLOAT4>, std::vector<DirectX::XMFLOAT4X4>,
    std::string>;

#endif
