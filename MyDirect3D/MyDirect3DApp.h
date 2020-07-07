#pragma once

#include "Direct3DApp.h"

class MyDirect3DApp : public Direct3DApp
{
public:
	MyDirect3DApp(HINSTANCE Instance);
	~MyDirect3DApp();

public:
	virtual bool Initialize() override;
};
