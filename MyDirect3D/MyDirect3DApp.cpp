#include "MyPch.h"
#include "MyDirect3DApp.h"

MyDirect3DApp::MyDirect3DApp(HINSTANCE Instance) :
	Direct3DApp(Instance)
{

}

MyDirect3DApp::~MyDirect3DApp()
{

}

bool MyDirect3DApp::Initialize()
{
	if (!Direct3DApp::Initialize())
	{
		return false;
	}

	return true;
}
