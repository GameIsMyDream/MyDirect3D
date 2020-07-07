#pragma once

#if defined (DEBUG) || defined (_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "Direct3DUtility.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

/**
 * Direct3D Ӧ�ó�����ࡣ�����ࡣ
 */
class Direct3DApp
{
protected:
	// ���캯����
	explicit Direct3DApp(HINSTANCE Instance);

	// ��ɾ�����������캯����
	Direct3DApp(const Direct3DApp& Rhs) = delete;

	// ��ɾ�����ƶ����캯����
	Direct3DApp(Direct3DApp&& Rhs) = delete;

	// ��ɾ����������ֵ�������
	Direct3DApp& operator=(const Direct3DApp& Rhs) = delete;

	// ��ɾ�����ƶ���ֵ�������
	Direct3DApp& operator=(Direct3DApp&& Rhs) = delete;

	// ����������
	virtual ~Direct3DApp();

public:
	// ��ʼ��Direct3DӦ�ó���
	virtual bool Initialize();

	// ��������Ϣѭ����
	int Run();

	// ��ȡ����ָ�롣
	static Direct3DApp* Get();

	// ��ȡӦ�ó���ʵ�������
	HINSTANCE GetAppInstance() const;

	// ��ȡ�����ھ����
	HWND GetMainWindow() const;

	// ��ȡ�����ڹ�������߱ȡ�
	float GetClientAspectRatio() const;

	// ��ȡ4�����ز��������״̬��
	bool Get4xMsaaState() const;

	// ����4�����ز��������״̬��
	void Set4xMsaaState(bool bState);

protected:
	// ��������ʼ�������ڡ�
	bool InitMainWindow();

	bool InitDirect3D(); // ��ʼ�� Direct3D��
	void LogAdapters(); // ��¼��ʾ��������
	void LogAdapterOutputs(IDXGIAdapter* Adapter); // ��¼��ʾ����������豸��
	void LogOutputDisplayModes(IDXGIOutput* Output, DXGI_FORMAT Format); // ��¼��ʾ����������豸����ʾģʽ��
	void CreateCommandObjects(); // �����������
	void CreateSwapChain(); // ������������
	virtual void CreateRtvAndDsvDescriptorHeaps(); // ���� RTV �� DSV �������ѡ�
	virtual void FlushCommandQueue(); // ˢ��������С�
	ID3D12Resource* GetCurrentBackBuffer() const; // ��ȡ��ǰ��̨��������

	 // �����ڳ���
	static LRESULT CALLBACK MainWindowProcedure(HWND Wnd, UINT Msg, WPARAM WParam, LPARAM LParam);

protected:
	static Direct3DApp* Singleton;		// ����ָ��
	HINSTANCE AppInstance;			// Ӧ�ó���ʵ�����
	HWND MainWindow;				// �����ھ��
	std::wstring MainWindowClassName;	// ����������
	std::wstring MainWindowName;	// ��������
	LONG ClientWidth;				// �����ڹ��������
	LONG ClientHeight;				// �����ڹ������߶�

	static const int SwapChainBufferCount = 2;	// ����������������
	int CurrentBackBuffer;						// ��ǰ��̨����������

	Microsoft::WRL::ComPtr<IDXGIFactory7> DxgiFactory;								// DXGI ����
	Microsoft::WRL::ComPtr<ID3D12Device> Device;									// D3D12 �豸
	Microsoft::WRL::ComPtr<ID3D12Fence> Fence;										// D3D12 Χ��
	Microsoft::WRL::ComPtr<ID3D12Resource> SwapChainBuffers[SwapChainBufferCount];	// ����������������

	bool b4xMsaaState;						// 4�����ز��������״̬
	UINT Current4xMsaaQualityLevels;			// 4�����ز����������������

	DXGI_FORMAT BackBufferFormat;			// ��̨��������ʽ
	DXGI_FORMAT DepthStencilBufferFormat;	// ���ģ�建������ʽ

	UINT RtvDescriptorSize;					// RTV �������Ѵ�С
	UINT DsvDescriptorSize;					// DSV �������Ѵ�С
	UINT CbvSrvUavDescriptorSize;			// CBV��SRV �� UAV �������Ѵ�С
};
