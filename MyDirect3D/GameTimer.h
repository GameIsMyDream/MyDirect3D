#pragma once

/**
 * Direct3D ��ʱ���ࡣ
 */
class GameTimer
{
public:
	GameTimer();

public:
	// ���ü�ʱ������Ϣѭ��ǰ���á�
	void Reset();

	// ÿ֡���á�
	void Tick();

	// ֹͣ��ʱ��
	void Stop();

	// ��ʼ��ʱ��
	void Start();

	// ��ȡ�����ü�ʱʱ�̣�����ǰ֡��ʼʱ�̻��ѵ���������������ͣ���ѵ�������
	float GetTotalTime() const;

	// ��ȡÿ֡���ĵ�������
	float GetDeltaTime() const;

private:
	bool		bStoped;				// ��ǰ�Ƿ�ֹͣ��ʱ

	__int64		BaseCount;				// ���ü�ʱʱ�̵�Count����
	__int64		PreviousFrameCount;		// ��һ֡ʼʱʱ�̵�Count����
	__int64		CurrentFrameCount;		// ��ǰ֡ʼʱʱ�̵�Count����
	__int64		PausedCounts;			// ֹͣ��ʱ���ѵ�Count����
	__int64		StopCount;				// ���һ��ֹͣ��ʱʱ�̵�Count����

	double		SecondsPerCount;		// �� / Count
	double		DeltaSeconds;			// ÿ֡���ѵ�����
};
