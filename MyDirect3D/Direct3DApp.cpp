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

	MainWindowClass.cbSize = sizeof(MainWindowClass);									// �˽ṹ����ֽڴ�С
	MainWindowClass.style = CS_HREDRAW | CS_VREDRAW;									// ��ʽ
	MainWindowClass.lpfnWndProc = MainWindowProcedure;									// ���ڳ����ָ��
	MainWindowClass.cbClsExtra = 0;														// ϵͳΪ�˽ṹ�����Ķ����ֽ���
	MainWindowClass.cbWndExtra = 0;														// ϵͳΪ����ʵ������Ķ����ֽ���
	MainWindowClass.hInstance = AppInstance;											// ע�ᴰ�����Ӧ�ó����̬���ӿ��ʵ�����
	MainWindowClass.hIcon = LoadIcon(AppInstance, MAKEINTRESOURCE(IDI_ICON));			// ͼ��ľ��
	MainWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);								// �α�ľ��
	MainWindowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));	// ������ˢ�ľ��
	MainWindowClass.lpszMenuName = NULL;												// �˵���Դ��
	MainWindowClass.lpszClassName = MainWindowClassName.c_str();						// ����
	MainWindowClass.hIconSm = NULL;														// Сͼ��ľ��

	// ע�ᴰ����
	if (!RegisterClassEx(&MainWindowClass))
	{
		wstring Text = L"RegisterClass failed, error code: " + to_wstring(GetLastError()) + L".";

		MessageBox(NULL, Text.c_str(), L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	RECT WindowRect = { 0, 0, ClientWidth, ClientHeight };
	LONG WindowWidth = WindowRect.right - WindowRect.left;
	LONG WindowHeight = WindowRect.bottom - WindowRect.top;

	// ���ݹ��������ε�������С�����㴰�ھ�������Ĵ�С
	AdjustWindowRectEx(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE, NULL);

	// ��������
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

	// ��ʾ���򴰿ڳ�����һ�� WM_PAINT ��Ϣ
	ShowWindow(MainWindow, SW_SHOW);

	// ���������ڵĹ�����
	UpdateWindow(MainWindow);

	return true;
}

bool Direct3DApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG)
	ComPtr<ID3D12Debug> DebugController;
	THROW_IF_FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController)));
	DebugController->EnableDebugLayer();
#endif // ���� D3D12 ���Բ㡣

	HRESULT HardwareResult = S_OK;
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS QualityLevels = {};

	// ���� DXGI ������
	THROW_IF_FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&DxgiFactory)));

	// �����豸��������ʾ��ʾ����������һ���������� nullptr ʹ��Ĭ����������IDXGIFactory1::EnumAdapters ����ö�ٵĵ�һ������������
	HardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&Device));
	// �������豸ʧ�ܣ���ö�� WARP��Windows Advanced Rasterization Platform��Windows �߼���դ��ƽ̨����Ϊ�������������豸��
	if (FAILED(HardwareResult))
	{
		ComPtr<IDXGIAdapter4> WarpAdapter;

		THROW_IF_FAILED(DxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&WarpAdapter)));
		THROW_IF_FAILED(D3D12CreateDevice(WarpAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&Device)));
	}

	// ����Χ����
	THROW_IF_FAILED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));

	// ��ȡ�������ѵľ��������С��
	RtvDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	DsvDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	CbvSrvUavDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// ��鵱ǰͼ����������֧�ֵĶ��ز���������������Ϣ��
	QualityLevels.Format = BackBufferFormat;
	QualityLevels.SampleCount = 4;
	QualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	QualityLevels.NumQualityLevels = 0;

	THROW_IF_FAILED(Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &QualityLevels, sizeof(QualityLevels)));
	Current4xMsaaQualityLevels = QualityLevels.NumQualityLevels;
	assert(Current4xMsaaQualityLevels > 0 && "Unexpected MSAA quality levels.");

#ifdef _DEBUG
	LogAdapters();
#endif // ����ǰ��������Ϣ��ӡ��������ڡ�

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

	// ��������ö�ٱ���ϵͳ�е���������
	while (DxgiFactory->EnumAdapters(Index, &Adapter) != DXGI_ERROR_NOT_FOUND)
	{
		Adapter->GetDesc(&AdapterDesc);

		Text = L"[Adapter " + to_wstring(Index) + L"]: ";
		Text += AdapterDesc.Description;
		Text += L"\n";

		OutputDebugString(Text.c_str());

		LogAdapterOutputs(Adapter); // ��¼���������������Ϣ��

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

	// ��������ö���������������
	while (Adapter->EnumOutputs(Index, &Output) != DXGI_ERROR_NOT_FOUND)
	{
		Output->GetDesc(&OutputDesc);

		Text = L" [Output " + to_wstring(Index) + L"]: ";
		Text += OutputDesc.DeviceName;;
		Text += L"\n";

		OutputDebugString(Text.c_str());

		LogOutputDisplayModes(Output, BackBufferFormat); // ��¼���������ʾģʽ��Ϣ��

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

	Output->GetDisplayModeList(Format, Flags, &ModeCount, nullptr); // ��ȡ��ʾģʽ��������

	ModeList.resize(ModeCount);

	Output->GetDisplayModeList(Format, Flags, &ModeCount, &ModeList[0]); // ��ȡ��ʾģʽ���б�

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

	// ����������С�
	THROW_IF_FAILED(Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(&CommandQueue)));

	// ���������������
	THROW_IF_FAILED(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&DirectCommandAllocator)));

	// ���������б�
	THROW_IF_FAILED(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, DirectCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&CommandList)));

	// ��һ�����������б�ʱ��Ҫ���ã�������֮ǰ��Ҫ�ر�����
	CommandList->Close();
}

void Direct3DApp::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};

	// �����´���������֮ǰ���ͷ�֮ǰ�Ľ�������
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

	// ������������
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

	// ���� RTV �������ѡ�
	THROW_IF_FAILED(Device->CreateDescriptorHeap(&RtvHeapDesc, IID_PPV_ARGS(&RtvHeap)));

	DsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	DsvHeapDesc.NumDescriptors = 1;
	DsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DsvHeapDesc.NodeMask = 0;

	// ���� DSV �������ѡ�
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
