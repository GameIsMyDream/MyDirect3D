#include "MyPch.h"
#include "resource.h"
#include "Direct3DApp.h"
//#include <WindowsX.h>

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
	CurrentBackBuffer(0),
	b4xMsaaState(false),
	Current4xMsaaQualityLevels(0),
	BackBufferFormat(DXGI_FORMAT_R8G8B8A8_UNORM),
	DepthStencilBufferFormat(DXGI_FORMAT_D24_UNORM_S8_UINT),
	RtvDescriptorSize(0),
	DsvDescriptorSize(0),
	CbvSrvUavDescriptorSize(0)
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
	MSG Msg = { 0 };

	while (Msg.message != WM_QUIT)
	{
		if (PeekMessage(&Msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
		else
		{
			;
		}
	}

	return static_cast<int>(Msg.wParam);
}

bool Direct3DApp::InitMainWindow()
{
	WNDCLASSEX MainWindowClass = {};

	MainWindowClass.cbSize = sizeof(MainWindowClass);									// 此结构体的字节大小
	MainWindowClass.style = CS_HREDRAW | CS_VREDRAW;									// 样式
	MainWindowClass.lpfnWndProc = MainWindowProcedure;									// 窗口程序的指针
	MainWindowClass.cbClsExtra = 0;														// 系统为此结构体分配的额外字节数
	MainWindowClass.cbWndExtra = 0;														// 系统为窗口实例分配的额外字节数
	MainWindowClass.hInstance = AppInstance;											// 注册窗口类的应用程序或动态链接库的实例句柄
	MainWindowClass.hIcon = LoadIcon(AppInstance, MAKEINTRESOURCE(IDI_ICON));			// 图标的句柄
	MainWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);								// 游标的句柄
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

	RECT WindowRect = { 0, 0, ClientWidth, ClientHeight };
	LONG WindowWidth = WindowRect.right - WindowRect.left;
	LONG WindowHeight = WindowRect.bottom - WindowRect.top;

	// 根据工作区矩形的期望大小，计算窗口矩形所需的大小
	AdjustWindowRectEx(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE, NULL);

	// 创建窗口
	MainWindow = CreateWindowEx(
		NULL,
		MainWindowClassName.c_str(),
		MainWindowName.c_str(),
		WS_OVERLAPPEDWINDOW,
		static_cast<int>((GetSystemMetrics(SM_CXSCREEN) - WindowWidth) / 2),
		static_cast<int>((GetSystemMetrics(SM_CYSCREEN) - WindowHeight) / 2),
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

	HRESULT HardwareResult = S_OK;
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS QualityLevels = {};

	// 创建 DXGI 工厂。
	THROW_IF_FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&DxgiFactory)));

	// 创建设备，用来表示显示适配器。第一个参数传递 nullptr 使用默认适配器（IDXGIFactory1::EnumAdapters 函数枚举的第一个适配器）。
	HardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&Device));
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
	DXGI_ADAPTER_DESC AdapterDesc = {};
	wstring Text = L"";

	// 根据索引枚举本地系统中的适配器。
	while (DxgiFactory->EnumAdapters(Index, &Adapter) != DXGI_ERROR_NOT_FOUND)
	{
		Adapter->GetDesc(&AdapterDesc);

		Text = L"[Adapter " + to_wstring(Index) + L"]: ";
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
	DXGI_OUTPUT_DESC OutputDesc = {};
	wstring Text = L"";

	// 根据索引枚举适配器的输出。
	while (Adapter->EnumOutputs(Index, &Output) != DXGI_ERROR_NOT_FOUND)
	{
		Output->GetDesc(&OutputDesc);

		Text = L" [Output " + to_wstring(Index) + L"]: ";
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
	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};

	// 在重新创建交换链之前，释放之前的交换链。
	SwapChain.Reset();

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

	// 创建交换链。
	THROW_IF_FAILED(DxgiFactory->CreateSwapChain(CommandQueue.Get(), &SwapChainDesc, &SwapChain));
}

void Direct3DApp::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC RtvHeapDesc = {};
	D3D12_DESCRIPTOR_HEAP_DESC DsvHeapDesc = {};

	RtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	RtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	RtvHeapDesc.NodeMask = 0;

	// 创建 RTV 描述符堆。
	THROW_IF_FAILED(Device->CreateDescriptorHeap(&RtvHeapDesc, IID_PPV_ARGS(&RtvHeap)));

	DsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	DsvHeapDesc.NumDescriptors = 1;
	DsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DsvHeapDesc.NodeMask = 0;

	// 创建 DSV 描述符堆。
	THROW_IF_FAILED(Device->CreateDescriptorHeap(&DsvHeapDesc, IID_PPV_ARGS(&DsvHeap)));
}

void Direct3DApp::OnResize()
{

}

void Direct3DApp::FlushCommandQueue()
{

}

ID3D12Resource* Direct3DApp::GetCurrentBackBuffer() const
{
	return SwapChainBuffers[CurrentBackBuffer].Get();
}

LRESULT Direct3DApp::MainWindowProcedure(HWND Wnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	switch (Msg)
	{
	case WM_ACTIVATE:
		return 0;

	case WM_SIZE:
		return 0;

	case WM_ENTERSIZEMOVE:
		return 0;

	case WM_EXITSIZEMOVE:
		return 0;

	case WM_DESTROY:
		return 0;

	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);

	case WM_GETMINMAXINFO:
		((MINMAXINFO*)LParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)LParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:

	case WM_MBUTTONDOWN:

	case WM_RBUTTONDOWN:
		return 0;

	case WM_LBUTTONUP:

	case WM_MBUTTONUP:

	case WM_RBUTTONUP:
		return 0;

	case WM_MOUSEMOVE:
		return 0;

	case WM_KEYUP:
		return 0;

	default:
		break;
	}

	return DefWindowProc(Wnd, Msg, WParam, LParam);
}
