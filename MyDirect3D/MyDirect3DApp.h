#pragma once

#include "Direct3DApp.h"

/**
 * Direct3D Ӧ�ó����ࡣ
 */
class MyDirect3DApp : public Direct3DApp
{
public:
	MyDirect3DApp(HINSTANCE Instance);
	~MyDirect3DApp();

public:
	virtual bool Initialize() override;
	virtual void Update(const GameTimer& Timer) override;
	virtual void Draw(const GameTimer& Timer) override;
	virtual void OnResize() override;
};
