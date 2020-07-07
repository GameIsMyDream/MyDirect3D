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

	bool InitDirect3D(); // 初始化 Direct3D。
	void LogAdapters(); // 记录显示适配器。
	void LogAdapterOutputs(IDXGIAdapter* Adapter); // 记录显示适配器输出设备。
	void LogOutputDisplayModes(IDXGIOutput* Output, DXGI_FORMAT Format); // 记录显示适配器输出设备的显示模式。
	void CreateCommandObjects(); // 创建命令对象。
	void CreateSwapChain(); // 创建交换链。
	virtual void CreateRtvAndDsvDescriptorHeaps(); // 创建 RTV 和 DSV 描述符堆。
	virtual void FlushCommandQueue(); // 刷新命令队列。
	ID3D12Resource* GetCurrentBackBuffer() const; // 获取当前后台缓冲区。

	 // 主窗口程序。
	static LRESULT CALLBACK MainWindowProcedure(HWND Wnd, UINT Msg, WPARAM WParam, LPARAM LParam);

protected:
	static Direct3DApp* Singleton;		// 单例指针
	HINSTANCE AppInstance;			// 应用程序实例句柄
	HWND MainWindow;				// 主窗口句柄
	std::wstring MainWindowClassName;	// 主窗口类名
	std::wstring MainWindowName;	// 主窗口名
	LONG ClientWidth;				// 主窗口工作区宽度
	LONG ClientHeight;				// 主窗口工作区高度

	static const int SwapChainBufferCount = 2;	// 交换链缓冲区数量
	int CurrentBackBuffer;						// 当前后台缓冲区索引

	Microsoft::WRL::ComPtr<IDXGIFactory7> DxgiFactory;								// DXGI 工厂
	Microsoft::WRL::ComPtr<ID3D12Device> Device;									// D3D12 设备
	Microsoft::WRL::ComPtr<ID3D12Fence> Fence;										// D3D12 围栏
	Microsoft::WRL::ComPtr<ID3D12Resource> SwapChainBuffers[SwapChainBufferCount];	// 交换链缓冲区数组

	bool b4xMsaaState;						// 4倍多重采样抗锯齿状态
	UINT Current4xMsaaQualityLevels;			// 4倍多重采样抗锯齿质量级别

	DXGI_FORMAT BackBufferFormat;			// 后台缓冲区格式
	DXGI_FORMAT DepthStencilBufferFormat;	// 深度模板缓冲区格式

	UINT RtvDescriptorSize;					// RTV 描述符堆大小
	UINT DsvDescriptorSize;					// DSV 描述符堆大小
	UINT CbvSrvUavDescriptorSize;			// CBV、SRV 和 UAV 描述符堆大小
};
