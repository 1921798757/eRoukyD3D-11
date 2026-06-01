#include "WinMin.h"
#include "CpuTimer.h"

CpuTimer::CpuTimer()
{
    __int64 countsPerSec{};
                                                                // LARGE_INTEGER是一个结构体，包含一个64位的整数，用于存储高精度计时器的频率和计数值。我们将其强制转换为__int64类型来获取计数值。
                                                                // 这是历史原因，QueryPerformanceFrequency和QueryPerformanceCounter函数使用LARGE_INTEGER类型来返回结果，但我们更习惯使用__int64类型来处理这些值，所以这里进行了类型转换。
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);   // 用于获取高精度计时器的频率，即每秒钟的计数次数。这个函数会将结果存储在countsPerSec变量中，单位是计数/秒。
    m_SecondsPerCount = 1.0 / (double)countsPerSec;             // 计算每个计数的秒数，即1秒钟有多少个计数。这个值将用于将计数转换为秒。
}


float CpuTimer::TotalTime()const
{
    // 如果调用了Stop()，暂停中的这段时间我们不需要计入。此外
    // m_StopTime - m_BaseTime可能会包含之前的暂停时间，为
    // 此我们可以从m_StopTime减去之前累积的暂停的时间
    //
    //                     |<-- 暂停的时间 -->|
    // ----*---------------*-----------------*------------*------------*------> time
    //  m_BaseTime       m_StopTime        startTime     m_StopTime    m_CurrTime

    if( m_Stopped )
    {
        return (float)(((m_StopTime - m_PausedTime)-m_BaseTime)*m_SecondsPerCount);
    }

    // m_CurrTime - m_BaseTime包含暂停时间，但我们不想将它计入。
    // 为此我们可以从m_CurrTime减去之前累积的暂停的时间
    //
    //  (m_CurrTime - m_PausedTime) - m_BaseTime 
    //
    //                     |<-- 暂停的时间 -->|
    // ----*---------------*-----------------*------------*------> time
    //  m_BaseTime       m_StopTime        startTime     m_CurrTime
    
    else
    {
        return (float)(((m_CurrTime-m_PausedTime)-m_BaseTime)*m_SecondsPerCount);
    }
}

float CpuTimer::DeltaTime()const
{
    return (float)m_DeltaTime;
}

void CpuTimer::Reset()
{
    __int64 currTime{};             // {}大括号初始化，这是C++11引入的“统一初始化”语法。这句等同于 __int64 currTime = 0;，但使用大括号初始化可以防止一些类型转换错误，并且在某些情况下可以更清晰地表达意图。对于内置类型，使用{}初始化会将其值初始化为0。
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime); // 获取当前的计数值，单位是计数。这个值表示从系统启动到现在经过的计数次数。

    m_BaseTime = currTime;
    m_PrevTime = currTime;
    m_StopTime = 0;
    m_PausedTime = 0;   // 涉及到多次Reset的话需要将其归0
    m_Stopped  = false;
}

void CpuTimer::Start()
{
    __int64 startTime{};
    QueryPerformanceCounter((LARGE_INTEGER*)&startTime);


    // 累积暂停开始到暂停结束的这段时间
    //
    //                     |<-------d------->|
    // ----*---------------*-----------------*------------> time
    //  m_BaseTime       m_StopTime        startTime     

    if( m_Stopped )
    {
        m_PausedTime += (startTime - m_StopTime);

        m_PrevTime = startTime;
        m_StopTime = 0;
        m_Stopped  = false;
    }
}

void CpuTimer::Stop()
{
    if( !m_Stopped )
    {
        __int64 currTime{};
        QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

        m_StopTime = currTime;
        m_Stopped  = true;
    }
}

void CpuTimer::Tick()
{
    if( m_Stopped )
    {
        m_DeltaTime = 0.0;
        return;
    }

    __int64 currTime{};
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
    m_CurrTime = currTime;

    // 当前Tick与上一Tick的帧间隔
    m_DeltaTime = (m_CurrTime - m_PrevTime)*m_SecondsPerCount;

    m_PrevTime = m_CurrTime;

    if(m_DeltaTime < 0.0)
    {
        m_DeltaTime = 0.0;
    }
}

bool CpuTimer::IsStopped() const
{
    return m_Stopped;
}

