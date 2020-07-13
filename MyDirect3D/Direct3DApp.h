#pragma once

#if defined (DEBUG) || defined (_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "GameTimer.h"
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

	// �������캯������ɾ������
	Direct3DApp(const Direct3DApp& Rhs) = delete;

	// �ƶ����캯������ɾ������
	Direct3DApp(Direct3DApp&& Rhs) = delete;

	// ������ֵ���������ɾ������
	Direct3DApp& operator=(const Direct3DApp& Rhs) = delete;

	// �ƶ���ֵ���������ɾ������
	Direct3DApp& operator=(Direct3DApp&& Rhs) = delete;

	// ����������
	virtual ~Direct3DApp();

public:
	// ��ʼ��Direct3DӦ�ó���
	virtual bool Initialize();

	// ��������Ϣѭ����
	int Run();

	// �����ڳ���
	LRESULT CALLBACK MainWindowProcedure(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

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

	// ��ʼ�� Direct3D��
	bool InitDirect3D();

	// ��¼��������Ϣ��
	void LogAdapters();

	// ��¼�����������Ϣ��
	void LogAdapterOutputs(IDXGIAdapter* Adapter);

	// ��¼�����ʾģʽ��Ϣ��
	void LogOutputDisplayModes(IDXGIOutput* Output, DXGI_FORMAT Format);

	// �����������
	void CreateCommandObjects();

	// ������������
	void CreateSwapChain();

	// ���� RTV �� DSV �������ѡ�
	virtual void CreateRtvAndDsvDescriptorHeaps();

	// ����֡ͳ����Ϣ��
	void CalculateFrameStats();

	// ���¼�ʱ����
	virtual void Update(const GameTimer& Timer) = 0;

	// ���ơ�
	virtual void Draw(const GameTimer& Timer) = 0;

	// �������ڴ�Сʱ���õĴ������
	virtual void OnResize();

	// ˢ��������С�
	virtual void FlushCommandQueue();

	// ���������ʱ���á�
	virtual void OnMouseButtonDown(WPARAM ButtonState, int ButtonX, int ButtonY) {};

	// ���ͷ����ʱ���á�
	virtual void OnMouseButtonUp(WPARAM ButtonState, int ButtonX, int ButtonY) {};

	// ������ƶ�ʱ���á�
	virtual void OnMouseButtonMove(WPARAM ButtonState, int ButtonX, int ButtonY) {};

	// ��ȡ��ǰ��̨��������
	ID3D12Resource* GetCurrentBackBuffer() const;

	// ��ȡ��ǰ��̨��������������
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;

	// ��ȡ��ǰ���/ģ�建������������
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentDepthStencilBufferView() const;

protected:
	static Direct3DApp*	Singleton;				// ����ָ��
	HINSTANCE			AppInstance;			// Ӧ�ó���ʵ�����
	HWND				MainWindow;				// �����ھ��
	std::wstring		MainWindowClassName;	// ����������
	std::wstring		MainWindowName;			// ��������
	UINT				ClientWidth;			// �����ڹ��������
	UINT				ClientHeight;			// �����ڹ������߶�
	bool				bAppPaused;				// ��ʾӦ�ó����Ƿ���ͣ
	bool				bMinimized;				// ��ʾ�����Ƿ��ѱ���С��
	bool				bMaximized;				// ��ʾ�����Ƿ��ѱ����
	bool				bResizing;				// ��ʾ���ڴ�С�Ƿ����ڱ�����
	GameTimer			Timer;					// ��ʱ��

	UINT				CurrentBackBufferIndex;			// ��ǰ��̨����������
	bool				b4xMsaaState;					// 4�����ز��������״̬
	UINT				NumberOf4xMsaaQualityLevels;	// 4�����ز����������������
	UINT				RtvDescriptorSize;				// RTV����ȾĿ����ͼ����������С
	UINT				DsvDescriptorSize;				// DSV�����/ģ����ͼ����������С
	UINT				CbvSrvUavDescriptorSize;		// CBV��������������ͼ����SRV����ɫ����Դ��ͼ���� UAV�����������ͼ����������С
	DXGI_FORMAT			BackBufferFormat;				// ��̨��������ʽ
	DXGI_FORMAT			DepthStencilBufferFormat;		// ���ģ�建������ʽ
	UINT64				CurrentFence;					// ��ǰ��Χ��ֵ
	static const UINT	SwapChainBufferCount = 2;		// ����������������
	D3D12_VIEWPORT		ScreenViewport;					// ��Ļ�ӿ�
	D3D12_RECT			ScreenRect;						// ��Ļ����

	Microsoft::WRL::ComPtr<IDXGIFactory4>				DxgiFactory;							// DXGI ����
	Microsoft::WRL::ComPtr<ID3D12Device>				Device;									// �豸
	Microsoft::WRL::ComPtr<ID3D12Fence>					Fence;									// Χ��
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>			CommandQueue;							// �������
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		DirectCommandAllocator;					// ֱ�����������
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	CommandList;							// �����б�
	Microsoft::WRL::ComPtr<IDXGISwapChain>				SwapChain;								// ������
	Microsoft::WRL::ComPtr<ID3D12Resource>				SwapChainBuffers[SwapChainBufferCount];	// ����������������
	Microsoft::WRL::ComPtr<ID3D12Resource>				DepthStencilBuffer;						// ���/ģ�建����
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		RtvHeap;								// RTV ��������
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		DsvHeap;								// DSV ��������
};

LRESULT CALLBACK Direct3DAppWindowProcedure(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);
