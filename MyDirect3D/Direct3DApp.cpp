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

	// ���ü�ʱ����
	Timer.Reset();

	// ִ����Ϣѭ����
	while (Message.message != WM_QUIT)
	{
		// ����������Ϣ
		if (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE)) // ����Ϣ�������д�����Ϣ��
		{
			// ������Ϣ��
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
		else // ����
		{
			// ִ�ж�������Ϸ�߼���
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
	case WM_ACTIVATE: // �����ڱ������ͣ��ʱ��
		if (LOWORD(WParam) == WA_INACTIVE) // �����ڱ�ͣ��ʱ��
		{
			// ��ͣ��Ϸ��
			bAppPaused = true;
			Timer.Stop();
		}
		else // �����ڱ�����ʱ��
		{
			// ������Ϸ��
			bAppPaused = false;
			Timer.Start();
		}

		return 0;

	case WM_SIZE: // �����ڴ�С���ı�ʱ��
		// �����µĴ��ڹ������Ĵ�С��
		ClientWidth = LOWORD(LParam);
		ClientHeight = LOWORD(LParam);

		if (Device)
		{
			if (WParam == SIZE_MINIMIZED) // �������ѱ���С����
			{
				bAppPaused = true;
				bMinimized = true;
				bMaximized = false;
			}
			else if (WParam == SIZE_MAXIMIZED) // �������ѱ���󻯣�
			{
				bAppPaused = false;
				bMinimized = false;
				bMaximized = true;

				OnResize();
			}
			else if (WParam == SIZE_RESTORED) // �����ڵĴ�С�ѱ��ı䣬����С������󻯵�ֵ�������ã�
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
				else // ���������
				{
					OnResize();
				}
			}
		}

		return 0;

	case WM_ENTERSIZEMOVE: // ���ڽ����ƶ��������Сģ̬ѭ��ʱ��
		bAppPaused = true;
		bResizing = true;

		Timer.Stop();

		return 0;

	case WM_EXITSIZEMOVE: // �����˳��ƶ��������Сģ̬ѭ��ʱ��
		bAppPaused = false;
		bResizing = false;

		Timer.Stop();
		OnResize();

		return 0;

	case WM_DESTROY: // ���ڱ�����ʱ��
		PostQuitMessage(0);

		return 0;

	case WM_MENUCHAR: // �˵����ڻ״̬���û��������κ����Ƿ�����ټ�����Ӧ�ļ�ʱ��
		return MAKELRESULT(0, MNC_CLOSE);

	case WM_GETMINMAXINFO: // ���ڵĴ�С��λ�ü�������ʱ��
		 reinterpret_cast<MINMAXINFO*>(LParam)->ptMinTrackSize.x = 200;
		 reinterpret_cast<MINMAXINFO*>(LParam)->ptMinTrackSize.y = 200;

		return 0;

	case WM_LBUTTONDOWN: // �ڴ��ڹ������ڰ���������ʱ��

	case WM_MBUTTONDOWN: // �ڴ��ڹ������ڰ�������м�ʱ��

	case WM_RBUTTONDOWN: // �ڴ��ڹ������ڰ�������Ҽ�ʱ��
		OnMouseButtonDown(WParam, GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam));

		return 0;

	case WM_LBUTTONUP: // �ڴ��ڹ��������ͷ�������ʱ��

	case WM_MBUTTONUP: // �ڴ��ڹ��������ͷ�����м�ʱ��

	case WM_RBUTTONUP: // �ڴ��ڹ��������ͷ�����Ҽ�ʱ��
		OnMouseButtonUp(WParam, GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam));

		return 0;

	case WM_MOUSEMOVE: // ����ƶ�ʱ��
		OnMouseButtonMove(WParam, GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam));

		return 0;

	case WM_KEYUP: // �ͷ�һ����ϵͳ��ʱ��
		if (WParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if (WParam == VK_F2)
		{
			Set4xMsaaState(!b4xMsaaState); // �л�4�����ز��������״̬��
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
	MainWindowClass.cbSize = sizeof(MainWindowClass);									// �˽ṹ����ֽڴ�С
	MainWindowClass.style = CS_HREDRAW | CS_VREDRAW;									// ��ʽ
	MainWindowClass.lpfnWndProc = Direct3DAppWindowProcedure;							// ���ڳ����ָ��
	MainWindowClass.cbClsExtra = 0;														// ϵͳΪ�˽ṹ�����Ķ����ֽ���
	MainWindowClass.cbWndExtra = 0;														// ϵͳΪ����ʵ������Ķ����ֽ���
	MainWindowClass.hInstance = AppInstance;											// ע�ᴰ�����Ӧ�ó����̬���ӿ��ʵ�����
	MainWindowClass.hIcon = LoadIcon(AppInstance, MAKEINTRESOURCE(IDI_ICON));			// ͼ��ľ��
	MainWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);								// ���ľ��
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

	// ���ݹ��������ε�������С�����㴰�ھ�������Ĵ�С
	RECT WindowRect = { 0, 0, static_cast<LONG>(ClientWidth), static_cast<LONG>(ClientHeight) };
	AdjustWindowRectEx(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE, NULL);

	// ��������
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

	// ���� DXGI ������
	THROW_IF_FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&DxgiFactory)));

	// �����豸��������ʾ��ʾ����������һ���������� nullptr ʹ��Ĭ����������IDXGIFactory1::EnumAdapters ����ö�ٵĵ�һ������������
	HRESULT HardwareResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&Device));
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

	// ��������ö�ٱ���ϵͳ�е���������
	while (DxgiFactory->EnumAdapters(Index, &Adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC AdapterDesc = {};
		Adapter->GetDesc(&AdapterDesc);

		wstring Text = L"[Adapter " + to_wstring(Index) + L"]: ";
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

	// ��������ö���������������
	while (Adapter->EnumOutputs(Index, &Output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC OutputDesc = {};
		Output->GetDesc(&OutputDesc);

		wstring Text = L" [Output " + to_wstring(Index) + L"]: ";
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
	// �����´���������֮ǰ���ͷ�֮ǰ�Ľ�������
	SwapChain.Reset();

	// ������������
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
	// ���� RTV �������ѡ�
	D3D12_DESCRIPTOR_HEAP_DESC RtvHeapDesc = {};
	RtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	RtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	RtvHeapDesc.NodeMask = 0;
	THROW_IF_FAILED(Device->CreateDescriptorHeap(&RtvHeapDesc, IID_PPV_ARGS(&RtvHeap)));

	// ���� DSV �������ѡ�
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
		float FramesPerSecond = static_cast<float>(FrameCount); // ���� FPS��֡�� / �롣
		float MillisecondsPerFrame = 1000.0f / FramesPerSecond; // ���� MSPF��1000 / FPS��

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

	// �ڸı���Դǰˢ��������С�
	FlushCommandQueue();

	// �������б�����Ϊ��ʼ״̬��
	THROW_IF_FAILED(CommandList->Reset(DirectCommandAllocator.Get(), nullptr));

	// �ͷ���Ҫ���´�������Դ��
	for (UINT i = 0; i != SwapChainBufferCount; ++i)
	{
		SwapChainBuffers[i].Reset();
	}
	DepthStencilBuffer.Reset();

	// �����������Ĵ�С
	THROW_IF_FAILED(SwapChain->ResizeBuffers(SwapChainBufferCount, ClientWidth, ClientHeight, BackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	CurrentBackBufferIndex = 0;

	// ��ȡ��ʾ����ʼλ�õ�CPU������������Դ�������һ�������ṹ�塣
	CD3DX12_CPU_DESCRIPTOR_HANDLE RtvHeapHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart());
	
	for (UINT i = 0; i != SwapChainBufferCount; ++i)
	{
		THROW_IF_FAILED(SwapChain->GetBuffer(i, IID_PPV_ARGS(&SwapChainBuffers[i])));
	
		Device->CreateRenderTargetView(SwapChainBuffers[i].Get(), nullptr, RtvHeapHandle);
	
		RtvHeapHandle.Offset(1, RtvDescriptorSize);
	}

	// �������/ģ�建��������������
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

	// �ȴ����ڴ�С�������
	FlushCommandQueue();

	// �����ӿڵ����
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

	// ����Χ��ֵ��
	THROW_IF_FAILED(CommandQueue->Signal(Fence.Get(), CurrentFence));

	// �����Ҫ����ȴ� GPU �������ﵱǰ��Χ��ֵ��
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
