#include "RenderBase.h"


RenderBase::RenderBase() : m_initialized(false), m_rtResourceWasSetBefore(false), m_rtvDescriptorSize(0)
{
	m_dsResource = nullptr;	

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
	m_psoManager = renderParams.PSOMngr;
	m_frameResourceManager= renderParams.FrameResourceMngr;
	m_dsResourceFormat = renderParams.DSResourceFormat;
	m_rtResourceFormat = renderParams.RTResourceFormat;
	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_initialized = true;

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
ID3D12Resource* RenderBase::create_Resource(DXGI_FORMAT resourceFormat, UINT width=0, UINT height=0)
{
	assert(m_initialized == true); // Render should be initialized before

	m_own_resources.resize(m_own_resources.size() + 1);

	UINT lwidth = width;
	UINT lheight = height;

	if (lwidth == 0) lwidth = m_width;
	if (lheight == 0) lheight= m_height;

	//Create Resource for any purposes
	D3D12_RESOURCE_DESC resourceDesriptor = {};
	resourceDesriptor.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesriptor.Alignment = 0;
	resourceDesriptor.Width = lwidth;
	resourceDesriptor.Height = lheight;
	resourceDesriptor.DepthOrArraySize = 1;
	resourceDesriptor.MipLevels = 1;
	resourceDesriptor.Format = resourceFormat;
	resourceDesriptor.SampleDesc.Count = 1;
	resourceDesriptor.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesriptor.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	HRESULT res = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&resourceDesriptor, D3D12_RESOURCE_STATE_COMMON, NULL, IID_PPV_ARGS(&m_own_resources[m_own_resources.size()-1]));
	
	return m_own_resources[m_own_resources.size() - 1].Get();
}

void RenderBase::create_Resource_DS(DXGI_FORMAT resourceFormat)
{
	m_dsResource = create_Resource(resourceFormat);
}

void RenderBase::create_Resource_RT(DXGI_FORMAT resourceFormat)
{
	m_rtResources.clear(); // if we have RTV resources previously set - claer it. Because we do not own it here, we do not need delete it.
	m_rtResources.push_back(create_Resource(resourceFormat));
	m_rtResourceWasSetBefore = false; // clear flag that RT resources could be set before.
}

void RenderBase::set_Resource_DS(ID3D12Resource* resource)
{
	m_dsResource = resource;
}

void RenderBase::set_Resource_RT(ID3D12Resource* resource)
{
	if (m_rtResourceWasSetBefore == false)
		m_rtResources.clear(); /*
		 Maybe we has RT Resources which were create by this render, 
		 but as we want to use RT resources from another source, we do not need current one
		 */

	m_rtResources.push_back(resource);
	m_rtResourceWasSetBefore = true;
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

void RenderBase::create_DescriptorHeap_RTV()
{
	// Create Descriptor heap for RenderTargetViews
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = m_rtResources.size();
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

void RenderBase::create_DSV(DXGI_FORMAT viewFormat)
{
	DXGI_FORMAT lViewFormat = viewFormat;
	if (lViewFormat == DXGI_FORMAT_UNKNOWN) lViewFormat = m_dsResourceFormat;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = lViewFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	m_device->CreateDepthStencilView(m_dsResource, &dsvDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	// Transition the resource from its initial state to be used as a depth buffer.
	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_dsResource,
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void RenderBase::create_RTV(DXGI_FORMAT viewFormat)
{
	DXGI_FORMAT lViewFormat = viewFormat;
	if (lViewFormat == DXGI_FORMAT_UNKNOWN) lViewFormat = m_rtResourceFormat;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());	
	for (int i = 0; i < m_rtResources.size(); i++)
	{		
		m_device->CreateRenderTargetView(m_rtResources[i], nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
	}
}

const ID3D12DescriptorHeap* RenderBase::get_dsvHeapPointer()
{
	return m_dsvHeap;
}

const ID3D12DescriptorHeap* RenderBase::get_rtvHeapPointer()
{
	return m_rtvHeap;
}

void RenderBase::set_DescriptorHeap(ID3D12DescriptorHeap* srvDescriptorHeap)
{
	assert(srvDescriptorHeap);
	m_descriptorHeap = srvDescriptorHeap;
}

// --------------- General Methods -------------------------------
void RenderBase::draw(int pFlags)
{

}