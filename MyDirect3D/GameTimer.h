#pragma once

/**
 * Direct3D 计时器类。
 */
class GameTimer
{
public:
	GameTimer();

public:
	// 重置计时。在消息循环前调用。
	void Reset();

	// 每帧调用。
	void Tick();

	// 停止计时。
	void Stop();

	// 开始计时。
	void Start();

	// 获取从重置计时时刻，到当前帧起始时刻花费的秒数，不包括暂停花费的秒数。
	float GetTotalTime() const;

	// 获取每帧消耗的秒数。
	float GetDeltaTime() const;

private:
	bool		bStoped;				// 当前是否停止计时

	__int64		BaseCount;				// 重置计时时刻的Count计数
	__int64		PreviousFrameCount;		// 上一帧始时时刻的Count计数
	__int64		CurrentFrameCount;		// 当前帧始时时刻的Count计数
	__int64		PausedCounts;			// 停止计时花费的Count计数
	__int64		StopCount;				// 最近一次停止计时时刻的Count计数

	double		SecondsPerCount;		// 秒 / Count
	double		DeltaSeconds;			// 每帧花费的秒数
};
