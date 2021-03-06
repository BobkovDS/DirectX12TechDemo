#include "RenderBase.h"

//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------
//---------------------------- MSAA RenderTarget ----------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------


void MSAARenderTargets::initialize(ID3D12Device* device, UINT width, UINT height,
	DXGI_FORMAT resourceFormat, D3D12_RESOURCE_FLAGS resourceFlags, D3D12_CLEAR_VALUE* optClear)
{
	m_resourceFormat = resourceFormat;
	m_resourceFlags = resourceFlags;
	m_device = device;

	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	m_msaaEnabled = true;
	m_msaaCount = 4;
	m_msaaQuality = 0;

	// Check Multisampling
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS lmsQualityLevels;
	lmsQualityLevels.SampleCount = m_msaaCount;
	lmsQualityLevels.Format = m_resourceFormat;
	lmsQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	lmsQualityLevels.NumQualityLevels = 0;
	if (SUCCEEDED(m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &lmsQualityLevels, sizeof(lmsQualityLevels))))
		m_msaaQuality = (lmsQualityLevels.NumQualityLevels > 0) ? (lmsQualityLevels.NumQualityLevels - 1) : 0;

	if (optClear)
	{
		m_optClear = new D3D12_CLEAR_VALUE(*optClear);
	}
	resize(width, height);
}

MSAARenderTargets::~MSAARenderTargets()
{
	if (m_optClear) delete m_optClear;
}

void MSAARenderTargets::resize(UINT width, UINT height)
{
	if (m_device == nullptr)
		return;

	for (int i = 0; i < g_swapChainsBufferCount2; i++)
	{
		RenderTargetBuffers[i].Reset();

		//Create Resource for any purposes
		D3D12_RESOURCE_DESC resourceDesriptor = {};
		resourceDesriptor.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesriptor.Alignment = 0;
		resourceDesriptor.Width = width;
		resourceDesriptor.Height = height;
		resourceDesriptor.DepthOrArraySize = 1;
		resourceDesriptor.MipLevels = 1;
		resourceDesriptor.Format = m_resourceFormat;
		resourceDesriptor.SampleDesc.Count = (m_msaaEnabled == true) ? m_msaaCount : 1;
		resourceDesriptor.SampleDesc.Quality = (m_msaaEnabled == true) ? m_msaaQuality : 0;
		resourceDesriptor.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesriptor.Flags = m_resourceFlags;

		HRESULT res = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
			&resourceDesriptor, D3D12_RESOURCE_STATE_RENDER_TARGET, m_optClear, IID_PPV_ARGS(&RenderTargetBuffers[i]));

#if defined(_DEBUG)
		std::wstring lResourceName = L"MSSARenderTargetResource" + std::to_wstring(i);
		RenderTargetBuffers[i]->SetName(lResourceName.c_str());
#endif

	}
}


void MSAARenderTargets::createRTV(ID3D12DescriptorHeap* RTVHeap)
{
	/* It uses the same RenderTarget Descriptor heap, which for DXGI SwapChain is used. So when we use
		MSAA Render targets, we re-write a RTV in this heap.
	*/

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < g_swapChainsBufferCount2; i++)
	{
		m_device->CreateRenderTargetView(RenderTargetBuffers[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
	}
}

void RenderResource::createResource(ID3D12Device* device, DXGI_FORMAT resourceFormat, D3D12_RESOURCE_FLAGS resourceFlags,
	UINT width, UINT height, DXGI_SAMPLE_DESC* sampleDesc, D3D12_CLEAR_VALUE* optClear, UINT16 texturesCount)
{
	m_resourceFormat = resourceFormat;
	m_resourceFlags = resourceFlags;
	m_texturesCount = texturesCount;
	m_device = device;
	if (optClear)
	{
		m_optClear = new D3D12_CLEAR_VALUE(*optClear);
	}

	if (sampleDesc != NULL)
	{
		m_sampleDesc.Count = sampleDesc->Count;
		m_sampleDesc.Quality = sampleDesc->Quality;
	}
	else
	{
		m_sampleDesc.Count = 1;
		m_sampleDesc.Quality = 0;
	}

	resize(width, height);
}

RenderResource::~RenderResource()
{
	if (m_optClear) delete m_optClear;
}

void RenderResource::resize(UINT width, UINT height)
{
	if (m_device == nullptr)
		return;
	
	m_resource.Reset();
			
	//Create Resource for any purposes
	D3D12_RESOURCE_DESC resourceDesriptor = {};
	resourceDesriptor.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesriptor.Alignment = 0;
	resourceDesriptor.Width = width;
	resourceDesriptor.Height = height;
	resourceDesriptor.DepthOrArraySize = m_texturesCount;
	resourceDesriptor.MipLevels = 1;
	resourceDesriptor.Format = m_resourceFormat;
	resourceDesriptor.SampleDesc = m_sampleDesc;
	resourceDesriptor.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesriptor.Flags = m_resourceFlags;
		
	HRESULT res = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&resourceDesriptor, D3D12_RESOURCE_STATE_COMMON, m_optClear, IID_PPV_ARGS(&m_resource));	
}

ID3D12Resource* RenderResource::getResource()
{
	return m_resource.Get();
}

//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------
//--------------------------------- RenderBase ------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------

RenderBase::RenderBase() : m_initialized(false), m_rtResourceWasSetBefore(false), m_doesRenderUseMutliSampling(false),
m_rtvDescriptorSize(0), m_currentResourceID(0) 
{
	m_dsResource = nullptr;	
	m_own_resources.resize(1);
}

RenderBase::~RenderBase()
{		
}

void RenderBase::initialize(const RenderMessager& renderParams)
{
	assert(m_initialized == false); //Render should not be initialized yet
	m_device = renderParams.Device;
	m_cmdList = renderParams.CmdList;
	m_width = renderParams.Width;
	m_height = renderParams.Height;
	m_scene= renderParams.Scene;	
	m_frameResourceManager= renderParams.FrameResourceMngr;
	m_resourceManager = renderParams.ResourceMngr;
	m_dsResourceFormat = renderParams.DSResourceFormat;
	m_rtResourceFormat = renderParams.RTResourceFormat;
	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_initialized = true;
	m_msaaRenderTargets = renderParams.MSAARenderTarget;

	resize(m_width, m_height);
}

void RenderBase::resize(UINT newWidth, UINT newHeight)
{
	m_width = newWidth;
	m_height = newHeight;

	m_viewPort = {};
	m_viewPort.Width = static_cast<float> (m_width);
	m_viewPort.Height= static_cast<float> (m_height);
	m_viewPort.MinDepth = 0.0f;
	m_viewPort.MaxDepth = 1.0f;

	m_scissorRect = { 0,0, static_cast<LONG> (m_width),static_cast<LONG> (m_height) };
}

// -------------------------------- RESOURCE -------------------------------------------------------------------------------
RenderResource* RenderBase::create_Resource(DXGI_FORMAT resourceFormat, D3D12_RESOURCE_FLAGS resourceFlags,
	UINT width, UINT height, DXGI_SAMPLE_DESC* sampleDesc, D3D12_CLEAR_VALUE* optClear, UINT16 texturesCount)
{
	assert(m_initialized == true); // Render should be initialized before
	
	UINT lwidth = width;
	UINT lheight = height;

	if (lwidth == 0) lwidth = m_width;
	if (lheight == 0) lheight= m_height;

	//Create Resource for any purposes

	m_own_resources[m_currentResourceID].createResource(m_device, resourceFormat, resourceFlags, lwidth, lheight, sampleDesc, optClear, texturesCount);

	return &m_own_resources[m_currentResourceID++];
}

void RenderBase::create_Resource_DS(DXGI_FORMAT resourceFormat)
{
	D3D12_CLEAR_VALUE optClear;
	optClear.Format = resourceFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;	

	DXGI_SAMPLE_DESC lSampleDesc;
	if (m_msaaRenderTargets!=NULL && m_msaaRenderTargets->getMSAAEnabled() && m_doesRenderUseMutliSampling)
	{
		lSampleDesc.Count = m_msaaRenderTargets->getSampleCount();
		lSampleDesc.Quality = m_msaaRenderTargets->getSampleQuality();
	}
	else
	{
		lSampleDesc.Count = 1;
		lSampleDesc.Quality= 0;
	}

	m_dsResource = create_Resource(resourceFormat, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, 0,0, &lSampleDesc, &optClear);
}

ID3D12Resource* RenderBase::get_Resource_DS()
{
	return m_dsResource->getResource();
}

void RenderBase::create_Resource_RT(DXGI_FORMAT resourceFormat, UINT width, UINT height, UINT16 resourcesCount, bool MultySamplingEnabled)
{
	FLOAT clearColor[4] = { 0.0f, 0.5f, 0.4f, 1.0f };
	D3D12_CLEAR_VALUE optClear;
	optClear.Format = resourceFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	std::copy(clearColor, clearColor + 4, optClear.Color);

	if (m_rtResourceWasSetBefore)
		m_rtResources.clear(); // if we have RTV resources previously set - claer it. Because we do not own it here, we do not need delete it.
	
	DXGI_SAMPLE_DESC lSampleDesc;
	if (m_msaaRenderTargets->getMSAAEnabled() && MultySamplingEnabled )
	{
		lSampleDesc.Count = m_msaaRenderTargets->getSampleCount();
		lSampleDesc.Quality = m_msaaRenderTargets->getSampleQuality();
	}
	else
	{
		lSampleDesc.Count = 1;
		lSampleDesc.Quality = 0;
	}

	m_rtResources.push_back(create_Resource(resourceFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, width, height, 
		&lSampleDesc, &optClear, resourcesCount));

	m_rtResourceWasSetBefore = false; // clear flag that RT resources could be set before.
}



// -------------------------------- DESCRIPTOR HEAP AND VIEWS --------------------------------------------------------------
void RenderBase::create_DescriptorHeap_DSV()
{
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.NumDescriptors = 1;	
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT res = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_own_dsvHeap));
	assert(SUCCEEDED(res));

	m_dsvHeap = m_own_dsvHeap.Get();
	m_dsvHeapWasSetBefore = false;
}

void RenderBase::create_DescriptorHeap_RTV(UINT descriptorsCount)
{
	// Create Descriptor heap for RenderTargetViews
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = descriptorsCount;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	
	HRESULT res = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_own_rtvHeap));
	assert(SUCCEEDED(res));

	m_rtvHeap = m_own_rtvHeap.Get();
	m_rtvHeapWasSetBefore = false;
}

void RenderBase::set_DescriptorHeap_RTV(ID3D12DescriptorHeap* rtvHeap)
{
	m_rtvHeap = rtvHeap;
	m_rtvHeapWasSetBefore = true;
}

void RenderBase::set_DescriptorHeap_DSV(ID3D12DescriptorHeap* dsvHeap)
{
	m_dsvHeap= dsvHeap;
	m_dsvHeapWasSetBefore = true;
}

void RenderBase::create_DSV(DXGI_FORMAT viewFormat)
{
	DXGI_FORMAT lViewFormat = viewFormat;
	if (lViewFormat == DXGI_FORMAT_UNKNOWN) lViewFormat = m_dsResourceFormat;

	bool lMutliSamplingEnabled = m_msaaRenderTargets != NULL && m_msaaRenderTargets->getMSAAEnabled() && m_doesRenderUseMutliSampling ? true : false;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = lMutliSamplingEnabled ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = lViewFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	m_device->CreateDepthStencilView(m_dsResource->getResource(), &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		
	// Transition the resource from its initial state to be used as a depth buffer.
	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_dsResource->getResource(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void RenderBase::create_RTV(DXGI_FORMAT viewFormat)
{
	DXGI_FORMAT lViewFormat = viewFormat;
	if (lViewFormat == DXGI_FORMAT_UNKNOWN) lViewFormat = m_rtResourceFormat;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());	
	for (int i = 0; i < m_rtResources.size(); i++)
	{		
		m_device->CreateRenderTargetView(m_rtResources[i]->getResource(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
	}

	/*Maybe here will be good place to change state for RT resources: to D3D12_RESOURCE_STATE_RENDER_TARGET*/
}

ID3D12DescriptorHeap* RenderBase::get_dsvHeapPointer()
{
	return m_dsvHeap;
}

ID3D12DescriptorHeap* RenderBase::get_rtvHeapPointer()
{
	return m_rtvHeap;
}

void RenderBase::set_DescriptorHeap(ID3D12DescriptorHeap* srvDescriptorHeap)
{
	assert(srvDescriptorHeap);
	m_descriptorHeap = srvDescriptorHeap;
}