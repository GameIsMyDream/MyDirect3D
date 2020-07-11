#include "MyPch.h"
#include "GameTimer.h"

GameTimer::GameTimer() :
	bStoped(false),
	BaseCount(0),
	PreviousFrameCount(0),
	CurrentFrameCount(0),
	PausedCounts(0),
	StopCount(0),
	SecondsPerCount(0.0),
	DeltaSeconds(-1.0)
{
	LARGE_INTEGER CountsPerSecond;
	QueryPerformanceFrequency(&CountsPerSecond);
	SecondsPerCount = 1.0 / static_cast<double>(CountsPerSecond.QuadPart);
}

void GameTimer::Reset()
{
	LARGE_INTEGER CurrentCount;
	QueryPerformanceCounter(&CurrentCount);

	bStoped = false;
	BaseCount = CurrentCount.QuadPart;
	PreviousFrameCount = CurrentCount.QuadPart;
	StopCount = 0;
}

void GameTimer::Tick()
{
	if (bStoped)
	{
		DeltaSeconds = 0.0;
		return;
	}

	LARGE_INTEGER CurrentCount;
	QueryPerformanceCounter(&CurrentCount);

	CurrentFrameCount = CurrentCount.QuadPart;
	DeltaSeconds = (CurrentFrameCount - PreviousFrameCount) * SecondsPerCount;
	PreviousFrameCount = CurrentFrameCount;

	// 强制 DeltaSeconds 为非负数。
	// 如果处理器进入低功耗模式，或者换到另一个处理器，那么 DeltaSeconds 可能为负数。
	if (DeltaSeconds < 0.0)
	{
		DeltaSeconds = 0.0;
	}
}

void GameTimer::Stop()
{
	if (!bStoped)
	{
		LARGE_INTEGER CurrentCount;
		QueryPerformanceCounter(&CurrentCount);

		StopCount = CurrentCount.QuadPart;
		bStoped = true;
	}
}

void GameTimer::Start()
{
	if (bStoped)
	{
		LARGE_INTEGER CurrentCount;
		QueryPerformanceCounter(&CurrentCount);

		PausedCounts += CurrentCount.QuadPart - StopCount;
		PreviousFrameCount = CurrentCount.QuadPart;
		StopCount = 0;
		bStoped = false;
	}
}

float GameTimer::GetTotalTime() const
{
	if (bStoped)
	{
		return static_cast<float>((StopCount - PausedCounts - BaseCount) * SecondsPerCount);
	}
	else
	{
		return static_cast<float>((CurrentFrameCount - PausedCounts - BaseCount) * SecondsPerCount);
	}
}

float GameTimer::GetDeltaTime() const
{
	return static_cast<float>(DeltaSeconds);
}
