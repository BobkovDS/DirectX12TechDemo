#include "BasicDXGI.h"
#include <iostream>

using namespace std; 

BasicDXGI::BasicDXGI(HINSTANCE hInstance, const std::wstring& applName, int width, int height)
	:Canvas(hInstance,  applName, width, height)
{
	m_fullScreen = false;
	m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;	

	m_rtSampleDesc.Count = 4;

	LOG("BasicDXGI canvas was created");
}

BasicDXGI::~BasicDXGI()
{
	FlushCommandQueue();

	{
		ComPtr<IDXGIDebug1> dxgiDebug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
		{
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);			
		}
	}

	LOG("BasicDXGI canvas was destroyed");
}

void BasicDXGI::run()
{
	Canvas::run();
	// -------------------- NO CODE AFTER THIS !!! ---------------
}

void BasicDXGI::work()
{
	draw();
}

void BasicDXGI::draw()
{
	m_cmdAllocator->Reset();
	const float clearColor[4] = { 0.0f, 0.0f, 0.6f, 1.0f };
	
	int cri = m_swapChain->GetCurrentBackBufferIndex();

	CD3DX12_CPU_DESCRIPTOR_HANDLE currentRTV(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_swapChain->GetCurrentBackBufferIndex(), 
		m_rtvDescriptorSize);

	HRESULT res;
	res = m_cmdList->Reset(m_cmdAllocator.Get(), nullptr);
	assert(SUCCEEDED(res));

	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainBuffers[cri].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_cmdList->ClearRenderTargetView(currentRTV, clearColor, 0, nullptr);
	m_cmdList->OMSetRenderTargets(1, &currentRTV, true, nullptr);

	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainBuffers[cri].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	m_cmdList->Close();

	ID3D12CommandList* CmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, CmdLists);

	FlushCommandQueue();

	if (m_prevPresentCallStatus != DXGI_STATUS_OCCLUDED)
	{
		//LOG("normal Present");
		m_prevPresentCallStatus = m_swapChain->Present(0, 0);		
	}
	else
	{
		//LOG("test Present");		
		m_prevPresentCallStatus = m_swapChain->Present(0, DXGI_PRESENT_TEST);
	}		
}

void BasicDXGI::init3D()
{
	LOG("BasicDXGI::init() output");
	HRESULT res = 0;
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug1> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	// Create Factory
	{
		res = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory));
		assert(SUCCEEDED(res));
	}

	// Get required Adapter/ Create Device
	{
		// Get required Adapter
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		getHardwareAdapter(m_factory.Get(), hardwareAdapter);

#if defined (_LOGDEBUG)
		logAdapters();
#endif

		// Create Device	
		res = D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device));		
		assert(SUCCEEDED(res));
	}

	// Create Fence and Event Handle for it
	{
		m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
		m_fenceValue = 0;

		m_fenceEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		assert(m_fenceEvent != nullptr);
	}

	//Create Command Objects;
	{
		//Command queue
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		res = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_cmdQueue));
		assert(SUCCEEDED(res));

		//Command list allocator
		res = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator));
		assert(SUCCEEDED(res));

		// Create Command List (here we think that one Command list is enough for us)
		res = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAllocator.Get(), 
			nullptr, IID_PPV_ARGS(&m_cmdList));
		assert(SUCCEEDED(res));

		m_cmdList->Close();
	}

	// Create SwapChain (SwapChain, Resourses for it and RenderTargetViews for last one in DescriptorHeap)
	{ 
		// Create SwapChain itself

		//m_swapChain.Reset(); use if Swapchain need re-create

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = width();// *m_dxKoef;
		swapChainDesc.Height = height();// *m_dyKoef;
		swapChainDesc.BufferCount = g_swapChainsBufferCount;
		swapChainDesc.Format = m_backBufferFormat;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;// m_rtSampleDesc.Count;
		swapChainDesc.SampleDesc.Quality = 0;// lmsQualityLevels.NumQualityLevels > 0 ? lmsQualityLevels.NumQualityLevels - 1 : 0;

		ComPtr<IDXGISwapChain1> pSwapChain;		
		res = m_factory->CreateSwapChainForHwnd(m_cmdQueue.Get(), hMainWind(), &swapChainDesc,
			nullptr, nullptr, &pSwapChain);
		assert(SUCCEEDED(res));
		pSwapChain.As(&m_swapChain);

		// Disable the Alt+Enter fullscreen toggle feature.
		res = m_factory->MakeWindowAssociation(hMainWind(), DXGI_MWA_NO_ALT_ENTER);
		
		// Create Descriptor heap for RenderTargetViews
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};		

		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.NumDescriptors = g_swapChainsBufferCount;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		res = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
		assert(SUCCEEDED(res));	

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		
#ifdef GUI_HUD
		// Direct2D
		{
			{
				// Create Direct2D Device - {Not sure about the place for this. It should be another way to clean it}
				UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
				ComPtr<ID3D11Device> ld3d11Device;
				res = D3D11On12CreateDevice(
					m_device.Get(),
					creationFlags,
					nullptr,
					0,
					reinterpret_cast<IUnknown**> (m_cmdQueue.GetAddressOf()),
					1,
					0,
					&ld3d11Device,
					&m_d3d11Context,
					nullptr);
				res = ld3d11Device.As(&m_d3d11On12Device);
			}

			// Create a Direct2D Factory		
			{
				//if (m_HUDContext)	m_HUDContext->Release();

				D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
				res = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_HUDFactory.GetAddressOf());
				ComPtr<IDXGIDevice> ldxgiDevice;
				res = m_d3d11On12Device.As(&ldxgiDevice);
				res = m_HUDFactory->CreateDevice(ldxgiDevice.Get(), &m_HUDDevice);
				res = m_HUDDevice->CreateDeviceContext(deviceOptions, &m_HUDContext);
				res = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &m_writeFactory);
			}

			// Create D2D/DWrite objects for rendering text
			{
				res = m_HUDContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::GreenYellow), &m_HUDBrush);
				m_writeFactory->CreateTextFormat(
					L"Consolas",
					NULL,
					DWRITE_FONT_WEIGHT_NORMAL,
					DWRITE_FONT_STYLE_NORMAL,
					DWRITE_FONT_STRETCH_NORMAL,
					18,
					L"en-us",
					&m_textFormat);
				res = m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
				res = m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

				m_writeFactory->CreateTextFormat(
					L"Consolas",
					NULL,
					DWRITE_FONT_WEIGHT_NORMAL,
					DWRITE_FONT_STYLE_NORMAL,
					DWRITE_FONT_STRETCH_NORMAL,
					30,
					L"en-us",
					&m_textFormatBig);
				res = m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
				res = m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

			}
		}
#endif	

		create_RTV();
		initDXGI_RTV_done = true;
	}	

	initDXGI_Full_done = true;
}

void BasicDXGI::create_RTV()
{
	assert(m_device);
	assert(m_swapChain);
	assert(m_cmdAllocator);

	FlushCommandQueue();

	//Reset Command List
	HRESULT res = 0;
	res = m_cmdList->Reset(m_cmdAllocator.Get(), nullptr);
	assert(SUCCEEDED(res));

	// Reset SwapChain buffers (Release the previous resources)
	for (int i = 0; i < g_swapChainsBufferCount; i++)
	{
		//if (m_swapChainBuffers[i])			m_swapChainBuffers[i].Get()->Release();		
		m_swapChainBuffers[i].Reset();
		m_HUDRenderTargets[i].Reset();
		m_wrappedBackBuffers[i].Reset();
	}

	if (m_d3d11Context)
		m_d3d11Context->Flush();

	// Resize SwapChain
	/* MSDN:
	Before you call ResizeBuffers, ensure that the application releases all references(by calling the appropriate
	number of Release invocations) on the resources, any views to the resource, and any command lists that use either
	the resources or views, and ensure that neither the resource nor a view is still bound to a device context.	.
	*/	

	res = m_swapChain->ResizeBuffers(g_swapChainsBufferCount, width() , height(),
		m_backBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
	assert(SUCCEEDED(res));	

	// Extract SwapChain buffers to be able to change a status for this and create RTV for this
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (int i = 0; i < g_swapChainsBufferCount; i++)
	{		
		m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffers[i]));	
		m_device->CreateRenderTargetView(m_swapChainBuffers[i].Get(), nullptr, rtvHeapHandle);		
		rtvHeapHandle.Offset(1, m_rtvDescriptorSize);

#ifdef GUI_HUD
		// for Direct2D
		float ldpiX, ldpiY;
		m_HUDFactory->GetDesktopDpi(&ldpiX, &ldpiY);
		D2D1_BITMAP_PROPERTIES1 lBitmapProperties = D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(m_backBufferFormat, D2D1_ALPHA_MODE_PREMULTIPLIED),
			ldpiX, ldpiY);

		D3D11_RESOURCE_FLAGS ld3d11Flags = { D3D11_BIND_RENDER_TARGET };
		res = m_d3d11On12Device->CreateWrappedResource(m_swapChainBuffers[i].Get(),
			&ld3d11Flags,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT,
			IID_PPV_ARGS(&m_wrappedBackBuffers[i]));

		//create a render target for D2D to draw directly to this back buffer
		ComPtr<IDXGISurface> lSurface;
		res = m_wrappedBackBuffers[i].As(&lSurface);
		res = m_HUDContext->CreateBitmapFromDxgiSurface(lSurface.Get(), &lBitmapProperties, &m_HUDRenderTargets[i]);
#endif
	}

	// Execute the resize commands
	res = m_cmdList->Close();
	
	ID3D12CommandList* cmdsList[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, cmdsList);
	
	//Wait when resize is done
	FlushCommandQueue(); 

	LOG("SwapChain was created/resized");
	assert(SUCCEEDED(res));
}

void BasicDXGI::FlushCommandQueue()
{
	/*
		Lets wait for when all command to this point are done in command queue.
		It is reuqired for that all resources and commands in commandAllocator
		are used and can be cleaned (reseted)
	*/

	m_fenceValue++;

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	m_cmdQueue->Signal(m_fence.Get(), m_fenceValue);

	//Wait until the GPU has completed up to this new fence point
	if (m_fence->GetCompletedValue() < m_fenceValue)
	{
		m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

void BasicDXGI::setFence()
{
	m_cmdQueue->Signal(m_fence.Get(), m_fenceValue); 

	m_fenceValue++;
}

void BasicDXGI::getHardwareAdapter(IDXGIFactory2* pFactory, ComPtr<IDXGIAdapter1>& ppAdapter)
{	
	if (pFactory == nullptr) return;
	
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &ppAdapter);
		++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		ppAdapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// do not select the Basic Render Device adapter; 
			continue;
		}

		// Check to see if the adapter support Dirtect3D 12, but don't create the actual device yet
		if (SUCCEEDED(D3D12CreateDevice(ppAdapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}		
}

void BasicDXGI::onKeyDown(WPARAM btnState) {
	switch (btnState) {
	case 'F':
	{
		m_fullScreen = !m_fullScreen; // DO_TO: Does it work?
		m_swapChain->SetFullscreenState(m_fullScreen, NULL);
	}
		return;
	}
}

void BasicDXGI::onReSize(int newWidth, int newHeight) {

	Canvas::onReSize(newWidth, newHeight); // save new width and height
	
	if (!initDXGI_RTV_done) return; // onResize call it before how 3D part was init
	create_RTV();	
}
//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------
//---------------------------- LOG FUNCTIONs --------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------

void BasicDXGI::logAdapters()
{
	ComPtr<IDXGIAdapter1> adapter;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		
		LOG(text);
		
		logAdapterOutput(adapter.Get());
	}
}

void BasicDXGI::logAdapterOutput(IDXGIAdapter1* pAdapter)
{
	UINT i = 0;
	IDXGIOutput* pOutput = nullptr;

	while (pAdapter->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		pOutput->GetDesc(&desc);

		std::wstring text = L"*****Output: ";
		text += desc.DeviceName;		

		LOG(text.c_str());

		logOutputDisplayMode(pOutput, m_backBufferFormat);

		ReleaseCom(pOutput);
		i++;
	}
}

void BasicDXGI::logOutputDisplayMode(IDXGIOutput* pOutput, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	// call with null parametr to get list count
	pOutput->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	pOutput->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) ;

		LOG(text.c_str());
	}
}
