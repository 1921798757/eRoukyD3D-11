#pragma once

#ifndef CPU_TIMER_H
#define CPU_TIMER_H

class CpuTimer
{
public:
    CpuTimer();
 
    float TotalTime()const;     // 返回从Reset()调用之后经过的时间，但不包括暂停期间的
    float DeltaTime()const;     // 返回帧间隔时间

    void Reset();               // 计时开始前或者需要重置时调用
    void Start();               // 在开始计时或取消暂停的时候调用
    void Stop();                // 在需要暂停的时候调用
    void Tick();                // 在每一帧开始的时候调用
    bool IsStopped() const;     // 计时器是否暂停/结束

private:
    double m_SecondsPerCount = 0.0;
    double m_DeltaTime = -1.0;

    __int64 m_BaseTime = 0;         // 刚启动或者调用Reset()时的那一瞬的滴答数
    __int64 m_PausedTime = 0;       // 总暂停时长
    __int64 m_StopTime = 0;         // 按下暂停键（切出游戏窗口）那一瞬间的滴答数
    __int64 m_PrevTime = 0;         // 上一帧的时间点的滴答数，用来与当前帧相减，算出dt
    __int64 m_CurrTime = 0;         // 当前时间点的滴答数，每一帧都要更新这个值。它与m_PrevTime的差值就是每一帧的时间增量（dt），用来驱动动画和游戏逻辑，使得它们与帧率无关。

    bool m_Stopped = false;
};

#endif // GAMETIMER_H