#include "Timer.h"
#include <Windows.h>
Timer::Timer()
{
	__int64 countPerSecond;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countPerSecond);
	m_SecondsPerCount = 1.0f / (double)countPerSecond; // how our Computer fast is

	QueryPerformanceCounter((LARGE_INTEGER*)&m_CurrTime);
	m_PrevTime = m_CurrTime;	
	m_totalTimerIsRunning = false;

	setTickTime(0.01f);
}

Timer::~Timer()
{
}

void Timer::begin()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&m_PrevTimeBE);
}

double Timer::end() // in seconds
{
	QueryPerformanceCounter((LARGE_INTEGER*)&m_CurrTime);
	return (m_CurrTime - m_PrevTimeBE)*m_SecondsPerCount;
}

bool Timer::tick()
{
	bool result;
	QueryPerformanceCounter((LARGE_INTEGER*)&m_CurrTime);
	m_DeltaTime = (m_CurrTime - m_PrevTime)*m_SecondsPerCount;
	if (m_DeltaTime >= m_delta)
	{
		m_PrevTime = m_CurrTime;
		result = true;
	}
	else
		result = false;

	if (m_totalTimerIsPaused) return false;
	else return result;
}

bool Timer::tick1Sec()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&m_CurrTime);
	double deltaTime = (m_CurrTime - m_PrevTime1S)*m_SecondsPerCount;
	if (deltaTime >= 1.0f)
	{
		m_PrevTime1S = m_CurrTime;
		return true;
	}
	else
		return false;
}

float Timer::deltaTime()
{
	return (float)m_DeltaTime;
}
// ------------------ Total Timer --------------------
float Timer::totalTime()
{
	if (m_TimerBegin == 0) return 0;
	
	if (!m_totalTimerIsPaused)
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
		m_CommonTotalTime = (currTime - m_TimerBegin);	

		m_CommonTotalTime = m_CommonTotalTime - m_pausedTime;
	}

	return m_CommonTotalTime * m_SecondsPerCount;
}

void Timer::tt_RunStop()
{
	m_totalTimerIsRunning = !m_totalTimerIsRunning;

	if (m_totalTimerIsRunning)
		QueryPerformanceCounter((LARGE_INTEGER*)&m_TimerBegin);
	else
	{
		m_TimerBegin = 0;
		m_CommonTotalTime = 0;
		m_totalTimerIsPaused = false;
	}
}

void Timer::tt_Pause()
{
	m_totalTimerIsPaused = !m_totalTimerIsPaused;

	if (m_totalTimerIsPaused)
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&m_TimeWhenPaused);		
	}
	else
	{
		if (m_TimerBegin != 0)
		{
			QueryPerformanceCounter((LARGE_INTEGER*)&m_TimeWhenUnPaused);
			m_pausedTime += (m_TimeWhenUnPaused - m_TimeWhenPaused);			
		}
	}
}

// ------------ [End of] Total Timer ---------------------