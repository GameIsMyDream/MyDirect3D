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

	//OnResize();

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
	WNDCLASSEX MainWindowClass;

	MainWindowClass.cbSize = sizeof(WNDCLASSEX);									// 此结构体的字节大小
	MainWindowClass.style = CS_HREDRAW | CS_VREDRAW;								// 样式
	MainWindowClass.lpfnWndProc = MainWindowProcedure;									// 窗口程序的指针
	MainWindowClass.cbClsExtra = 0;													// 系统为此结构体分配的额外字节数
	MainWindowClass.cbWndExtra = 0;													// 系统为窗口实例分配的额外字节数
	MainWindowClass.hInstance = AppInstance;											// 注册窗口类的应用程序或动态链接库的实例句柄
	MainWindowClass.hIcon = LoadIcon(AppInstance, MAKEINTRESOURCE(IDI_ICON));	// 图标的句柄
	MainWindowClass.hCursor = LoadCursor(0, IDC_ARROW);								// 游标的句柄
	MainWindowClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);					// 背景画刷的句柄
	MainWindowClass.lpszMenuName = NULL;													// 菜单资源名
	MainWindowClass.lpszClassName = MainWindowClassName.c_str();							// 类名
	MainWindowClass.hIconSm = NULL;													// 小图标的句柄

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
	AdjustWindowRectEx(&WindowRect, WS_TILED, FALSE, NULL);

	// 创建窗口
	MainWindow = CreateWindowEx(
		NULL,
		MainWindowClassName.c_str(),
		MainWindowName.c_str(),
		WS_TILED,
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

	// 设定主窗口的显示状态
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
#endif // 启用 D3D12 调试层

	HRESULT HardwareResult = S_OK;

	// 创建 DXGI 工厂
	THROW_IF_FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&DxgiFactory)));

	// 创建表示显示适配器的设备
	HardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&Device));
	if (FAILED(HardwareResult))
	{
		ComPtr<IDXGIAdapter4> WarpAdapter;

		THROW_IF_FAILED(DxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&WarpAdapter))); // 枚举 WARP 适配器
		THROW_IF_FAILED(D3D12CreateDevice(WarpAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&Device)));
	}

	// 创建 D3D12 围栏
	THROW_IF_FAILED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));

	// 获取描述符堆的句柄增量的大小
	RtvDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	DsvDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	CbvSrvUavDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 获取当前图形驱动程序支持的功能级别的信息
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS QualityLevels;

	QualityLevels.Format = BackBufferFormat;
	QualityLevels.SampleCount = 4;
	QualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	QualityLevels.NumQualityLevels = 0;

	THROW_IF_FAILED(Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &QualityLevels, sizeof(QualityLevels)));
	Current4xMsaaQualityLevels = QualityLevels.NumQualityLevels;
	assert(Current4xMsaaQualityLevels > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
	LogAdapters();
#endif

	CreateCommandObjects();

	CreateSwapChain();

	CreateRtvAndDsvDescriptorHeaps();

	return true;
}

void Direct3DApp::LogAdapters()
{
	IDXGIAdapter* Adapter = nullptr;
	vector<IDXGIAdapter*> AdapterList;
	UINT Index = 0;

	while (DxgiFactory->EnumAdapters(Index++, &Adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC AdapterDesc;
		wstring Text = L"Adapter: ";

		Adapter->GetDesc(&AdapterDesc);
		Text += AdapterDesc.Description;
		Text += L"\n";

		OutputDebugString(Text.c_str());

		AdapterList.push_back(Adapter);
	}

	for (size_t i = 0; i != AdapterList.size(); ++i)
	{
		LogAdapterOutputs(AdapterList[i]);
		RELEASE_COM(AdapterList[i]);
	}
}

void Direct3DApp::LogAdapterOutputs(IDXGIAdapter* Adapter)
{
	IDXGIOutput* Output = nullptr;
	UINT Index = 0;

	while (Adapter->EnumOutputs(Index++, &Output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC OutputDesc;
		wstring Text = L"Output: ";

		Output->GetDesc(&OutputDesc);

		Text += OutputDesc.DeviceName;;
		Text += L"\n";

		OutputDebugString(Text.c_str());

		LogOutputDisplayModes(Output, BackBufferFormat);

		RELEASE_COM(Output);
	}
}

void Direct3DApp::LogOutputDisplayModes(IDXGIOutput* Output, DXGI_FORMAT Format)
{
	UINT Flags = 0;
	UINT ModeCount = 0;
	wstring Text = L"";

	Output->GetDisplayModeList(Format, Flags, &ModeCount, nullptr);

	vector<DXGI_MODE_DESC> ModeList(ModeCount);

	Output->GetDisplayModeList(Format, Flags, &ModeCount, &ModeList[0]);

	for (DXGI_MODE_DESC& Mode : ModeList)
	{
		Text = L"Width = " + to_wstring(Mode.Width) + L" Height = " + to_wstring(Mode.Height) +
			L" Refresh = " + to_wstring(Mode.RefreshRate.Numerator) + L" / " + to_wstring(Mode.RefreshRate.Denominator) + L"\n";

		OutputDebugString(Text.c_str());
	}
}

void Direct3DApp::CreateCommandObjects()
{

}

void Direct3DApp::CreateSwapChain()
{
}

void Direct3DApp::CreateRtvAndDsvDescriptorHeaps()
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
