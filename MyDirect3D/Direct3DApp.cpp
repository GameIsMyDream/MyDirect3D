#include "MyPch.h"
#include "resource.h"
#include "Direct3DApp.h"
#include <WindowsX.h>

using Microsoft::WRL::ComPtr;
//using namespace DirectX;
using namespace std;

Direct3DApp* Direct3DApp::Singleton = nullptr;

Direct3DApp::Direct3DApp(HINSTANCE Instance) :
	AppInstance(Instance),
	MainWindow(nullptr),
	MainWindowClassName(L"Direct3D Window"),
	MainWindowName(L"My Direct3D Window"),
	ClientWidth(960),
	ClientHeight(540),
	bAppPaused(false),
	bMinimized(false),
	bMaximized(false),
	bResizing(false),
	CurrentBackBufferIndex(0),
	b4xMsaaState(false),
	Current4xMsaaQualityLevels(0),
	RtvDescriptorSize(0),
	DsvDescriptorSize(0),
	CbvSrvUavDescriptorSize(0),
	BackBufferFormat(DXGI_FORMAT_R8G8B8A8_UNORM),
	DepthStencilBufferFormat(DXGI_FORMAT_D24_UNORM_S8_UINT),
	CurrentFence(0),
	ScreenViewport(D3D12_VIEWPORT()),
	ScreenRect(D3D12_RECT())
{
	assert(Singleton == nullptr);

	Singleton = this;
}

Direct3DApp::~Direct3DApp()
{

}

bool Direct3DApp::Initialize()
{
	if (!InitMainWindow())
	{
		return false;
	}

	if (!InitDirect3D())
	{
		return false;
	}

	OnResize();

	return true;
}

Direct3DApp* Direct3DApp::Get()
{
	return Singleton;
}

HINSTANCE Direct3DApp::GetAppInstance() const
{
	return AppInstance;
}

HWND Direct3DApp::GetMainWindow() const
{
	return MainWindow;
}

float Direct3DApp::GetClientAspectRatio() const
{
	return static_cast<float>(ClientWidth) / ClientHeight;
}

bool Direct3DApp::Get4xMsaaState() const
{
	return b4xMsaaState;
}

void Direct3DApp::Set4xMsaaState(bool bState)
{
	b4xMsaaState = bState;
}

int Direct3DApp::Run()
{
	MSG Message = {};

	// 重置计时器。
	Timer.Reset();

	// 执行消息循环。
	while (Message.message != WM_QUIT)
	{
		// 检索窗口消息
		if (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE)) // 若消息队列中有窗口消息：
		{
			// 处理消息。
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
		else // 否则：
		{
			// 执行动画或游戏逻辑。
			Timer.Tick();

			if (!bAppPaused)
			{
				CalculateFrameStats();
				Update(Timer);
				Draw(Timer);
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return static_cast<int>(Message.wParam);
}

LRESULT Direct3DApp::MainWindowProcedure(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	switch (Message)
	{
	case WM_ACTIVATE: // 当窗口被激活或被停用时：
		if (LOWORD(WParam) == WA_INACTIVE) // 当窗口被停用时：
		{
			// 暂停游戏。
			bAppPaused = true;
			Timer.Stop();
		}
		else // 当窗口被激活时：
		{
			// 继续游戏。
			bAppPaused = false;
			Timer.Start();
		}

		return 0;

	case WM_SIZE: // 当窗口大小被改变时：
		// 保存新的窗口工作区的大小。
		ClientWidth = LOWORD(LParam);
		ClientHeight = LOWORD(LParam);

		if (Device)
		{
			if (WParam == SIZE_MINIMIZED) // 若窗口已被最小化：
			{
				bAppPaused = true;
				bMinimized = true;
				bMaximized = false;
			}
			else if (WParam == SIZE_MAXIMIZED) // 若窗口已被最大化：
			{
				bAppPaused = false;
				bMinimized = false;
				bMaximized = true;

				OnResize();
			}
			else if (WParam == SIZE_RESTORED) // 若窗口的大小已被改变，但最小化和最大化的值都不适用：
			{
				if (bMinimized)
				{
					bAppPaused = false;
					bMinimized = false;

					OnResize();
				}
				else if (bMaximized)
				{
					bAppPaused = false;
					bMaximized = false;

					OnResize();
				}
				else if (bResizing)
				{
					;
				}
				else // 其他情况：
				{
					OnResize();
				}
			}
		}

		return 0;

	case WM_ENTERSIZEMOVE: // 窗口进入移动或调整大小模态循环时：
		bAppPaused = true;
		bResizing = true;

		Timer.Stop();

		return 0;

	case WM_EXITSIZEMOVE: // 窗口退出移动或调整大小模态循环时：
		bAppPaused = false;
		bResizing = false;

		Timer.Stop();
		OnResize();

		return 0;

	case WM_DESTROY: // 窗口被销毁时：
		PostQuitMessage(0);

		return 0;

	case WM_MENUCHAR: // 菜单处于活动状态且用户按下与任何助记符或加速键不对应的键时：
		return MAKELRESULT(0, MNC_CLOSE);

	case WM_GETMINMAXINFO: // 窗口的大小或位置即将更改时：
		 reinterpret_cast<MINMAXINFO*>(LParam)->ptMinTrackSize.x = 200;
		 reinterpret_cast<MINMAXINFO*>(LParam)->ptMinTrackSize.y = 200;

		return 0;

	case WM_LBUTTONDOWN: // 在窗口工作区内按下鼠标左键时：

	case WM_MBUTTONDOWN: // 在窗口工作区内按下鼠标中键时：

	case WM_RBUTTONDOWN: // 在窗口工作区内按下鼠标右键时：
		OnMouseButtonDown(WParam, GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam));

		return 0;

	case WM_LBUTTONUP: // 在窗口工作区内释放鼠标左键时：

	case WM_MBUTTONUP: // 在窗口工作区内释放鼠标中键时：

	case WM_RBUTTONUP: // 在窗口工作区内释放鼠标右键时：
		OnMouseButtonUp(WParam, GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam));

		return 0;

	case WM_MOUSEMOVE: // 光标移动时：
		OnMouseButtonMove(WParam, GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam));

		return 0;

	case WM_KEYUP: // 释放一个非系统键时：
		if (WParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if (WParam == VK_F2)
		{
			Set4xMsaaState(!b4xMsaaState); // 切换4倍多重采样抗锯齿状态。
		}

		return 0;

	default:
		break;
	}

	return DefWindowProc(Window, Message, WParam, LParam);
}

bool Direct3DApp::InitMainWindow()
{
	WNDCLASSEX MainWindowClass = {};
	MainWindowClass.cbSize = sizeof(MainWindowClass);									// 此结构体的字节大小
	MainWindowClass.style = CS_HREDRAW | CS_VREDRAW;									// 样式
	MainWindowClass.lpfnWndProc = Direct3DAppWindowProcedure;							// 窗口程序的指针
	MainWindowClass.cbClsExtra = 0;														// 系统为此结构体分配的额外字节数
	MainWindowClass.cbWndExtra = 0;														// 系统为窗口实例分配的额外字节数
	MainWindowClass.hInstance = AppInstance;											// 注册窗口类的应用程序或动态链接库的实例句柄
	MainWindowClass.hIcon = LoadIcon(AppInstance, MAKEINTRESOURCE(IDI_ICON));			// 图标的句柄
	MainWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);								// 光标的句柄
	MainWindowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));	// 背景画刷的句柄
	MainWindowClass.lpszMenuName = NULL;												// 菜单资源名
	MainWindowClass.lpszClassName = MainWindowClassName.c_str();						// 类名
	MainWindowClass.hIconSm = NULL;														// 小图标的句柄

	// 注册窗口类
	if (!RegisterClassEx(&MainWindowClass))
	{
		wstring Text = L"RegisterClass failed, error code: " + to_wstring(GetLastError()) + L".";

		MessageBox(NULL, Text.c_str(), L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	// 根据工作区矩形的期望大小，计算窗口矩形所需的大小
	RECT WindowRect = { 0, 0, static_cast<LONG>(ClientWidth), static_cast<LONG>(ClientHeight) };
	AdjustWindowRectEx(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE, NULL);

	// 创建窗口
	UINT WindowWidth = WindowRect.right - WindowRect.left;
	UINT WindowHeight = WindowRect.bottom - WindowRect.top;
	MainWindow = CreateWindowEx(
		NULL,
		MainWindowClassName.c_str(),
		MainWindowName.c_str(),
		WS_OVERLAPPEDWINDOW,
		(GetSystemMetrics(SM_CXSCREEN) - WindowWidth) / 2,
		(GetSystemMetrics(SM_CYSCREEN) - WindowHeight) / 2,
		WindowWidth,
		WindowHeight,
		NULL,
		NULL,
		AppInstance,
		NULL);

	if (!MainWindow)
	{
		wstring Text = L"CreateWindow failed, error code: " + to_wstring(GetLastError()) + L".";

		MessageBox(NULL, Text.c_str(), L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	// 显示并向窗口程序发送一个 WM_PAINT 消息
	ShowWindow(MainWindow, SW_SHOW);

	// 更新主窗口的工作区
	UpdateWindow(MainWindow);

	return true;
}

bool Direct3DApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG)
	ComPtr<ID3D12Debug> DebugController;
	THROW_IF_FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController)));
	DebugController->EnableDebugLayer();
#endif // 启用 D3D12 调试层。

	// 创建 DXGI 工厂。
	THROW_IF_FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&DxgiFactory)));

	// 创建设备，用来表示显示适配器。第一个参数传递 nullptr 使用默认适配器（IDXGIFactory1::EnumAdapters 函数枚举的第一个适配器）。
	HRESULT HardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&Device));
	// 若创建设备失败，则枚举 WARP（Windows Advanced Rasterization Platform，Windows 高级光栅化平台）作为适配器来创建设备。
	if (FAILED(HardwareResult))
	{
		ComPtr<IDXGIAdapter4> WarpAdapter;

		THROW_IF_FAILED(DxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&WarpAdapter)));
		THROW_IF_FAILED(D3D12CreateDevice(WarpAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&Device)));
	}

	// 创建围栏。
	THROW_IF_FAILED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));

	// 获取描述符堆的句柄增量大小。
	RtvDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	DsvDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	CbvSrvUavDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 检查当前图形驱动程序支持的多重采样的质量级别信息。
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS QualityLevels = {};
	QualityLevels.Format = BackBufferFormat;
	QualityLevels.SampleCount = 4;
	QualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	QualityLevels.NumQualityLevels = 0;

	THROW_IF_FAILED(Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &QualityLevels, sizeof(QualityLevels)));
	Current4xMsaaQualityLevels = QualityLevels.NumQualityLevels;
	assert(Current4xMsaaQualityLevels > 0 && "Unexpected MSAA quality levels.");

#ifdef _DEBUG
	LogAdapters();
#endif // 将当前适配器信息打印到输出窗口。

	CreateCommandObjects();

	CreateSwapChain();

	CreateRtvAndDsvDescriptorHeaps();

	return true;
}

void Direct3DApp::LogAdapters()
{
	UINT Index = 0;
	IDXGIAdapter* Adapter = nullptr;

	// 根据索引枚举本地系统中的适配器。
	while (DxgiFactory->EnumAdapters(Index, &Adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC AdapterDesc = {};
		Adapter->GetDesc(&AdapterDesc);

		wstring Text = L"[Adapter " + to_wstring(Index) + L"]: ";
		Text += AdapterDesc.Description;
		Text += L"\n";

		OutputDebugString(Text.c_str());

		LogAdapterOutputs(Adapter); // 记录该适配器的输出信息。

		RELEASE_COM(Adapter);

		++Index;
	}
}

void Direct3DApp::LogAdapterOutputs(IDXGIAdapter* Adapter)
{
	UINT Index = 0;
	IDXGIOutput* Output = nullptr;

	// 根据索引枚举适配器的输出。
	while (Adapter->EnumOutputs(Index, &Output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC OutputDesc = {};
		Output->GetDesc(&OutputDesc);

		wstring Text = L" [Output " + to_wstring(Index) + L"]: ";
		Text += OutputDesc.DeviceName;;
		Text += L"\n";

		OutputDebugString(Text.c_str());

		LogOutputDisplayModes(Output, BackBufferFormat); // 记录该输出的显示模式信息。

		RELEASE_COM(Output);

		++Index;
	}
}

void Direct3DApp::LogOutputDisplayModes(IDXGIOutput* Output, DXGI_FORMAT Format)
{
	UINT Flags = 0;
	UINT ModeCount = 0;
	wstring Text = L"";
	vector<DXGI_MODE_DESC> ModeList;

	Output->GetDisplayModeList(Format, Flags, &ModeCount, nullptr); // 获取显示模式的数量。

	ModeList.resize(ModeCount);

	Output->GetDisplayModeList(Format, Flags, &ModeCount, &ModeList[0]); // 获取显示模式的列表。

	for (DXGI_MODE_DESC& Mode : ModeList)
	{
		Text = L"  Width = " + to_wstring(Mode.Width) + L" Height = " + to_wstring(Mode.Height) +
			L" Refresh = " + to_wstring(Mode.RefreshRate.Numerator) + L" / " + to_wstring(Mode.RefreshRate.Denominator) + L"\n";

		OutputDebugString(Text.c_str());
	}
}

void Direct3DApp::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
	CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	// 创建命令队列。
	THROW_IF_FAILED(Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(&CommandQueue)));

	// 创建命令分配器。
	THROW_IF_FAILED(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&DirectCommandAllocator)));

	// 创建命令列表。
	THROW_IF_FAILED(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, DirectCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&CommandList)));

	// 第一次引用命令列表时需要重置，而重置之前需要关闭它。
	CommandList->Close();
}

void Direct3DApp::CreateSwapChain()
{
	// 在重新创建交换链之前，释放之前的交换链。
	SwapChain.Reset();

	// 创建交换链。
	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
	SwapChainDesc.BufferDesc.Width = ClientWidth;
	SwapChainDesc.BufferDesc.Height = ClientHeight;
	SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	SwapChainDesc.BufferDesc.Format = BackBufferFormat;
	SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	SwapChainDesc.SampleDesc.Count = b4xMsaaState ? 4 : 1;
	SwapChainDesc.SampleDesc.Quality = b4xMsaaState ? Current4xMsaaQualityLevels - 1 : 0;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = SwapChainBufferCount;
	SwapChainDesc.OutputWindow = MainWindow;
	SwapChainDesc.Windowed = TRUE;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	THROW_IF_FAILED(DxgiFactory->CreateSwapChain(CommandQueue.Get(), &SwapChainDesc, &SwapChain));
}

void Direct3DApp::CreateRtvAndDsvDescriptorHeaps()
{
	// 创建 RTV 描述符堆。
	D3D12_DESCRIPTOR_HEAP_DESC RtvHeapDesc = {};
	RtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	RtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	RtvHeapDesc.NodeMask = 0;
	THROW_IF_FAILED(Device->CreateDescriptorHeap(&RtvHeapDesc, IID_PPV_ARGS(&RtvHeap)));

	// 创建 DSV 描述符堆。
	D3D12_DESCRIPTOR_HEAP_DESC DsvHeapDesc = {};
	DsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	DsvHeapDesc.NumDescriptors = 1;
	DsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DsvHeapDesc.NodeMask = 0;
	THROW_IF_FAILED(Device->CreateDescriptorHeap(&DsvHeapDesc, IID_PPV_ARGS(&DsvHeap)));
}

void Direct3DApp::CalculateFrameStats()
{
	static UINT FrameCount = 0;
	static float TimeElapsed = 0.0f;

	++FrameCount;

	if (Timer.GetTotalTime() - TimeElapsed >= 1.0f)
	{
		float FramesPerSecond = static_cast<float>(FrameCount); // 计算 FPS：帧数 / 秒。
		float MillisecondsPerFrame = 1000.0f / FramesPerSecond; // 计算 MSPF：1000 / FPS。

		SetWindowText(MainWindow, (MainWindowName + L"        FPS: " + to_wstring(FramesPerSecond) + L"    MSPF: " + to_wstring(MillisecondsPerFrame)).c_str());

		FrameCount = 0;
		TimeElapsed += 1.0f;
	}
}

void Direct3DApp::OnResize()
{
	assert(Device);
	assert(SwapChain);
	assert(DirectCommandAllocator);

	// 在改变资源前刷新命令队列。
	FlushCommandQueue();

	// 将命令列表重置为初始状态。
	THROW_IF_FAILED(CommandList->Reset(DirectCommandAllocator.Get(), nullptr));

	// 释放需要重新创建的资源。
	for (UINT i = 0; i != SwapChainBufferCount; ++i)
	{
		SwapChainBuffers[i].Reset();
	}
	DepthStencilBuffer.Reset();

	// 调整交换链的大小
	THROW_IF_FAILED(SwapChain->ResizeBuffers(SwapChainBufferCount, ClientWidth, ClientHeight, BackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	CurrentBackBufferIndex = 0;

	// 获取表示堆起始位置的CPU描述符句柄，以此来创建一个辅助结构体。
	CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHeapHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart());
	
	for (UINT i = 0; i != SwapChainBufferCount; ++i)
	{
		THROW_IF_FAILED(SwapChain->GetBuffer(i, IID_PPV_ARGS(&SwapChainBuffers[i])));
	
		Device->CreateRenderTargetView(SwapChainBuffers[i].Get(), nullptr, RtvHeapHandle);
	
		RtvHeapHandle.Offset(1, RtvDescriptorSize);
	}

	// 创建深度/模板缓冲区和描述符。
	D3D12_RESOURCE_DESC DepthStencilDesc = {};
	DepthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	DepthStencilDesc.Alignment = 0;
	DepthStencilDesc.Width = ClientWidth;
	DepthStencilDesc.Height = ClientHeight;
	DepthStencilDesc.DepthOrArraySize = 1;
	DepthStencilDesc.MipLevels = 1;
	DepthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	DepthStencilDesc.SampleDesc.Count = b4xMsaaState ? 4 : 1;
	DepthStencilDesc.SampleDesc.Quality = b4xMsaaState ? (Current4xMsaaQualityLevels - 1) : 0;
	DepthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	DepthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE OptimizeClear = {};
	OptimizeClear.Format = DepthStencilBufferFormat;
	OptimizeClear.DepthStencil.Depth = 1.0f;
	OptimizeClear.DepthStencil.Stencil = 0;

	THROW_IF_FAILED(Device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
		D3D12_HEAP_FLAG_NONE, &DepthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &OptimizeClear, IID_PPV_ARGS(DepthStencilBuffer.GetAddressOf())));

	D3D12_DEPTH_STENCIL_VIEW_DESC DsvDesc = {};
	DsvDesc.Format = DepthStencilBufferFormat;
	DsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	DsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	DsvDesc.Texture2D.MipSlice = 0;
	Device->CreateDepthStencilView(DepthStencilBuffer.Get(), &DsvDesc, GetCurrentDepthStencilBufferView());

	CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(DepthStencilBuffer.Get(), 
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	THROW_IF_FAILED(CommandList->Close());

	ID3D12CommandList* CommandLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);

	// 等待窗口大小调整完成
	FlushCommandQueue();

	// 更新视口的外观
	ScreenViewport.TopLeftX = 0;
	ScreenViewport.TopLeftY = 0;
	ScreenViewport.Width = static_cast<float>(ClientWidth);
	ScreenViewport.Height = static_cast<float>(ClientHeight);
	ScreenViewport.MinDepth = 0.0f;
	ScreenViewport.MaxDepth = 1.0f;

	ScreenRect = { 0, 0, static_cast<LONG>(ClientWidth), static_cast<LONG>(ClientHeight) };
}

void Direct3DApp::FlushCommandQueue()
{
	++CurrentFence;

	// 更新围栏值。
	THROW_IF_FAILED(CommandQueue->Signal(Fence.Get(), CurrentFence));

	// 如果需要，则等待 GPU 完成命令到达当前的围栏值。
	if (Fence->GetCompletedValue() < CurrentFence)
	{
		HANDLE CommandCompletionEvent = CreateEventEx(nullptr, nullptr, NULL, EVENT_ALL_ACCESS);

		THROW_IF_FAILED(Fence->SetEventOnCompletion(CurrentFence, CommandCompletionEvent));

		WaitForSingleObject(CommandCompletionEvent, INFINITE);

		CloseHandle(CommandCompletionEvent);
	}
}

ID3D12Resource* Direct3DApp::GetCurrentBackBuffer() const
{
	return SwapChainBuffers[CurrentBackBufferIndex].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE Direct3DApp::GetCurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(RtvHeap->GetCPUDescriptorHandleForHeapStart(), CurrentBackBufferIndex, RtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE Direct3DApp::GetCurrentDepthStencilBufferView() const
{
	return DsvHeap->GetCPUDescriptorHandleForHeapStart();
}

LRESULT Direct3DAppWindowProcedure(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	return Direct3DApp::Get()->MainWindowProcedure(Window, Message, WParam, LParam);
}
