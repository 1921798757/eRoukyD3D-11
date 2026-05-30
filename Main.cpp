#include "GameApp.h"

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE prevInstance,
    _In_ LPSTR cmdLine,
    _In_ int showCmd)
{
    // 这些参数不使用
    UNREFERENCED_PARAMETER(prevInstance);
    UNREFERENCED_PARAMETER(cmdLine);
    UNREFERENCED_PARAMETER(showCmd);
    // 允许在Debug版本进行运行时内存分配和泄漏检测
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    GameApp theApp(hInstance, L"Rendering a Triangle", 1280, 720);

    if (!theApp.Init())
        return 0;

    return theApp.Run();//针对WinMain的return，因为要等待Run()执行完毕才有返回结果。故这里需要等待执行完毕。
                        //Run()中有while循环，当用户关闭窗口时，Run()才会返回，WinMain才会结束。
}