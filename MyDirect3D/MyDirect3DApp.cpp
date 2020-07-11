#include "MyPch.h"
#include "MyDirect3DApp.h"
#include <DirectXColors.h>

using namespace DirectX;

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

void MyDirect3DApp::Update(const GameTimer& Timer)
{

}

void MyDirect3DApp::Draw(const GameTimer& Timer)
{
	// �����������������������ڴ档
	THROW_IF_FAILED(DirectCommandAllocator->Reset());

	// ���������б�
	THROW_IF_FAILED(CommandList->Reset(DirectCommandAllocator.Get(), nullptr));

	// ֪ͨ��������ͬ������Դ�Ķ�����ʡ�
	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(), 
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// �����ӿ���ü����Ρ�
	CommandList->RSSetViewports(1, &ScreenViewport);
	CommandList->RSSetScissorRects(1, &ScreenRect);

	// �����̨�����������/ģ�建������
	CommandList->ClearRenderTargetView(GetCurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	CommandList->ClearDepthStencilView(GetCurrentDepthStencilBufferView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// ����CPU���������
	CommandList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), TRUE, &GetCurrentDepthStencilBufferView());

	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(), 
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	THROW_IF_FAILED(CommandList->Close());

	ID3D12CommandList* CommandLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);

	THROW_IF_FAILED(SwapChain->Present(0, 0));
	CurrentBackBufferIndex = (CurrentBackBufferIndex + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}

void MyDirect3DApp::OnResize()
{
	Direct3DApp::OnResize();
}
