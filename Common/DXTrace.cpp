#include "DXTrace.h"
#include <cstdio>



// ----------------------------------------------------------------------------------
//   DirectX 错误追踪与调试输出函数。
//   1. 格式化输出错误所在的文件名、行号、自定义错误信息及 HRESULT 错误码含义至调试输出窗口 (OutputDebugString)。
//   2. 可选弹出模态消息框通知开发者，并支持直接中断进入调试器 (DebugBreak)。
//
// 参数说明:
//   [in] strFile     : 产生错误的文件名，以 \0 结尾（通常传入宏 __FILEW__）。
//   [in] dwLine      : 产生错误的行号（通常传入宏 (DWORD)__LINE__）。
//   [in] hr          : 待追踪的 HRESULT 错误码。
//   [in] strMsg      : [可选] 附加的自定义信息/报错代码字符串（可传 nullptr）。
//   [in] bPopMsgBox  : 是否弹出错误提示消息框（true 弹出并允许断点调试，false 仅输出日志）。
//
// 返回值: 
//   传入的原样 HRESULT 错误代码，方便直接在宏或 return 语句中链式调用。
// ----------------------------------------------------------------------------------
HRESULT WINAPI DXTraceW(_In_z_ const WCHAR* strFile, _In_ DWORD dwLine, _In_ HRESULT hr,
    _In_opt_z_ const WCHAR* strMsg, _In_ bool bPopMsgBox)
{
    WCHAR strBufferFile[MAX_PATH];     // 存储文件名副本
    WCHAR strBufferLine[128];          // 存储行号字符串
    WCHAR strBufferError[300];         // 存储系统解析出的错误描述及十六进制错误码
    WCHAR strBufferMsg[1024];          // 存储格式化后的自定义描述信息
    WCHAR strBufferHR[40];             // 存储十六进制错误代码文本 (0x00000000)
    WCHAR strBuffer[3000];             // 最终拼接输出的主缓冲区

    //转换并输出 [文件名(行号)] 前缀,  %lu无符号长整数（unsigned long / DWORD）
    swprintf_s(strBufferLine, 128, L"%lu", dwLine);
    //本质：判断指针是否有效（即 strFile != nullptr / strFile != NULL）
    if (strFile)//strFile 会是 NULL，完全取决于调用者传入了什么参数。调用这个函数时，已经出现了错误
    {
        swprintf_s(strBuffer, 3000, L"%ls(%ls): ", strFile, strBufferLine);
        OutputDebugStringW(strBuffer);//OutputDebugStringW 输出到 VS 的调试输出窗口
    }

    size_t nMsgLen = (strMsg) ? wcsnlen_s(strMsg, 1024) : 0;// wcs是 wide char string，wcsnlen_s 是安全的字符串长度计算函数，返回 strMsg 的长度，最大不超过 1024
    if (nMsgLen > 0)
    {
        OutputDebugStringW(strMsg);
        OutputDebugStringW(L" ");
    }
    // Windows SDK 8.0起DirectX的错误信息已经集成进错误码中，可以通过FormatMessageW获取错误信息字符串
    // 不需要分配字符串内存
    // -------------------------------------------------------------
    // 1. 将数字错误码 (hr) 翻译成人类可读的文字描述
    // -------------------------------------------------------------
    FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, // 告诉 Windows：“去系统错误库里查，并且忽略占位符”
        nullptr,                                                    // 消息来源（nullptr 表示使用系统默认库）
        hr,                                                         // 传入你要查询的错误代码（如 0x80004005）
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),                  // 使用操作系统的默认语言（中文系统返回中文，英文系统返回英文）
        strBufferError,                                             // 输出缓冲区：把翻译出来的文本写到这个数组里
        256,                                                        // 缓冲区最大容量（最多写 256 个字符，防止越界）
        nullptr                                                     // 额外参数列表，这里不需要
    );

    // -------------------------------------------------------------
    // 2. 清理系统返回文本自带的换行符
    // FormatMessage 查出来的字符串末尾默认会自带 "\r\n"，会导致排版错乱
    // -------------------------------------------------------------
    // wcsrchr: Wide Character String Reverse Character（从后往前查找字符）
    // 作用：在 strBufferError 里从后往前找最后一个回车符 '\r'
    WCHAR* errorStr = wcsrchr(strBufferError, L'\r');
    if (errorStr)
    {
        // 找到了就把 '\r' 换成 '\0'（字符串结束标记）
        // 这样就把后面的 "\r\n" 瞬间斩断了
        errorStr[0] = L'\0';    
    }

    // -------------------------------------------------------------
    // 3. 把十六进制错误码拼接到文字描述后面
    // -------------------------------------------------------------
    // 把数字 hr 格式化为 8 位的十六进制字符串，如 " (0x887a0005)"
    swprintf_s(strBufferHR, 40, L" (0x%0.8x)", hr);

    // wcscat_s: 拼接宽字符串（把十六进制码追加到刚刚翻译出的错误描述末尾）
    // 结果类似："拒绝访问。 (0x80070005)"
    wcscat_s(strBufferError, strBufferHR);

    // 组装最终日志并输出到 Visual Studio 调试器的“输出”窗口
    swprintf_s(strBuffer, 3000, L"错误码含义：%ls", strBufferError);
    OutputDebugStringW(strBuffer);
    OutputDebugStringW(L"\n"); // 换行

    // -------------------------------------------------------------
    // 4. 弹窗提示开发者（如果 bPopMsgBox 为 true）
    // -------------------------------------------------------------
    if (bPopMsgBox)
    {
        // 先清空 strBufferFile 缓冲区（赋空字符串 ""）
        wcscpy_s(strBufferFile, MAX_PATH, L"");
        if (strFile)
            wcscpy_s(strBufferFile, MAX_PATH, strFile); // 安全拷贝文件名

        // 先清空 strBufferMsg 缓冲区
        wcscpy_s(strBufferMsg, 1024, L"");
        if (nMsgLen > 0)
            swprintf_s(strBufferMsg, 1024, L"当前调用：%ls\n", strMsg); // 格式化自定义报错信息

        // 把所有信息（文件名、行号、错误含义、自定义调用代码）整合成一段长文本
        swprintf_s(strBuffer, 3000, L"文件名：%ls\n行号：%ls\n错误码含义：%ls\n%ls您需要调试当前应用程序吗？",
            strBufferFile, strBufferLine, strBufferError, strBufferMsg);

        // 弹出系统消息框：
        // GetForegroundWindow(): 获取当前获得焦点的窗口句柄，让弹窗置顶显示
        // MB_YESNO | MB_ICONERROR: 显示“是/否”两个按钮，并带一个红色 ❌ 错误图标
        int nResult = MessageBoxW(GetForegroundWindow(), strBuffer, L"错误", MB_YESNO | MB_ICONERROR);
        
        // 如果开发者点击了“是”按钮
        if (nResult == IDYES)
            DebugBreak(); // 触发硬中断！直接让程序在这一行暂停，跳出 VS 调试代码光标供你调试排查
    }

    return hr; // 把错误码原样返回
}