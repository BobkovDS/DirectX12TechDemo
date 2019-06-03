#pragma once
class Timer
{
private:
	double m_SecondsPerCount;
	double m_DeltaTime;	
	double m_CommonTotalTime = 0;
	double m_pausedTime=0;
	const double m_delta = 0.01;

	__int64 m_PrevTime = 0;
	__int64 m_CurrTime = 0;
	__int64 m_PrevTimeBE = 0; // for Begin and End methods		
	__int64 m_PrevTime1S = 0; // for 1 second timer
	__int64 m_TimerBegin = 0; // for Total timer
	__int64 m_TimeWhenPaused = 0; // for Total timer
	__int64 m_TimeWhenUnPaused = 0; // for Total timer

	bool m_totalTimerIsRunning = false;
	bool m_totalTimerIsPaused = false;
public:
	Timer();
	~Timer();

	void tt_RunStop();
	void tt_Pause();
	float totalTime();


	bool tick();
	bool tick1Sec();
	void begin();
	double end();
	float deltaTime();
};

