//***************************************************************************************
// WinMin.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 最小化Windows头文件冲突
// Minimize Windows header conflicts
//***************************************************************************************

#pragma once

#ifndef WINMIN_H
#define WINMIN_H

// 避免Windows.h包含一些不常用的头文件
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// 避免min/max宏与std::min/std::max冲突
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#endif // WINMIN_H
