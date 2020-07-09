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
 * Direct3D 应用程序基类。单例类。
 */
class Direct3DApp
{
protected:
	// 构造函数。
	explicit Direct3DApp(HINSTANCE Instance);

	// 已删除：拷贝构造函数。
	Direct3DApp(const Direct3DApp& Rhs) = delete;

	// 已删除：移动构造函数。
	Direct3DApp(Direct3DApp&& Rhs) = delete;

	// 已删除：拷贝赋值运算符。
	Direct3DApp& operator=(const Direct3DApp& Rhs) = delete;

	// 已删除：移动赋值运算符。
	Direct3DApp& operator=(Direct3DApp&& Rhs) = delete;

	// 析构函数。
	virtual ~Direct3DApp();

public:
	// 初始化Direct3D应用程序。
	virtual bool Initialize();

	// 主窗口消息循环。
	int Run();

	// 获取单例指针。
	static Direct3DApp* Get();

	// 获取应用程序实例句柄。
	HINSTANCE GetAppInstance() const;

	// 获取主窗口句柄。
	HWND GetMainWindow() const;

	// 获取主窗口工作区宽高比。
	float GetClientAspectRatio() const;

	// 获取4倍多重采样抗锯齿状态。
	bool Get4xMsaaState() const;

	// 设置4倍多重采样抗锯齿状态。
	void Set4xMsaaState(bool bState);

protected:
	// 创建并初始化主窗口。
	bool InitMainWindow();

	// 初始化 Direct3D。
	bool InitDirect3D();

	// 记录适配器信息。
	void LogAdapters();

	// 记录适配器输出信息。
	void LogAdapterOutputs(IDXGIAdapter* Adapter);

	// 记录输出显示模式信息。
	void LogOutputDisplayModes(IDXGIOutput* Output, DXGI_FORMAT Format);

	// 创建命令对象。
	void CreateCommandObjects();

	// 创建交换链。
	void CreateSwapChain();

	// 创建 RTV 和 DSV 描述符堆。
	virtual void CreateRtvAndDsvDescriptorHeaps();

	// 计算帧统计信息。
	void CalculateFrameStats();

	// 更新计时器。
	virtual void Update(const GameTimer& Timer) = 0;

	// 绘制。
	virtual void Draw(const GameTimer& Timer) = 0;

	// 调整窗口大小时调用的处理程序
	virtual void OnResize();

	// 刷新命令队列。
	virtual void FlushCommandQueue();

	// 获取当前后台缓冲区。
	ID3D12Resource* GetCurrentBackBuffer() const;

	// 主窗口程序。
	static LRESULT CALLBACK MainWindowProcedure(HWND Wnd, UINT Msg, WPARAM WParam, LPARAM LParam);

protected:
	static Direct3DApp*		Singleton;				// 单例指针
	HINSTANCE				AppInstance;			// 应用程序实例句柄
	HWND					MainWindow;				// 主窗口句柄
	std::wstring			MainWindowClassName;	// 主窗口类名
	std::wstring			MainWindowName;			// 主窗口名
	UINT					ClientWidth;			// 主窗口工作区宽度
	UINT					ClientHeight;			// 主窗口工作区高度
	bool					bAppPaused;				// 表示应用程序是否暂停
	GameTimer				Timer;					// 计时器

	UINT				CurrentBackBufferIndex;		// 当前后台缓冲区索引
	bool				b4xMsaaState;				// 4倍多重采样抗锯齿状态
	UINT				Current4xMsaaQualityLevels;	// 4倍多重采样抗锯齿质量级别
	UINT				RtvDescriptorSize;			// RTV（渲染目标视图）描述符大小
	UINT				DsvDescriptorSize;			// DSV（深度/模板视图）描述符大小
	UINT				CbvSrvUavDescriptorSize;	// CBV（常量缓冲区视图）、SRV（着色器资源视图）和 UAV（无序访问视图）描述符大小
	DXGI_FORMAT			BackBufferFormat;			// 后台缓冲区格式
	DXGI_FORMAT			DepthStencilBufferFormat;	// 深度模板缓冲区格式
	UINT64				CurrentFence;				// 当前的围栏值
	static const UINT	SwapChainBufferCount = 2;	// 交换链缓冲区数量

	Microsoft::WRL::ComPtr<IDXGIFactory7>				DxgiFactory;							// DXGI 工厂
	Microsoft::WRL::ComPtr<ID3D12Device>				Device;									// 设备
	Microsoft::WRL::ComPtr<ID3D12Fence>					Fence;									// 围栏
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>			CommandQueue;							// 命令队列
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		DirectCommandAllocator;					// 直接命令分配器
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	CommandList;							// 命令列表
	Microsoft::WRL::ComPtr<IDXGISwapChain>				SwapChain;								// 交换链
	Microsoft::WRL::ComPtr<ID3D12Resource>				SwapChainBuffers[SwapChainBufferCount];	// 交换链缓冲区数组
	Microsoft::WRL::ComPtr<ID3D12Resource>				DepthStencilBuffer;						// 深度/模板缓冲区
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		RtvHeap;								// RTV 描述符堆
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		DsvHeap;								// DSV 描述符堆
};
