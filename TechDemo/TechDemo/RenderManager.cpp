#include "RenderManager.h"

#define TECHSRVCOUNT 10 // see also in ResourceManager.cpp 

RenderManager::RenderManager(): m_initialized(false)
{
}

RenderManager::~RenderManager()
{
}

void RenderManager::initialize(const RenderManagerMessanger& renderParams)
{
	m_device = renderParams.commonRenderData.Device;
	m_cmdList= renderParams.commonRenderData.CmdList;
	m_swapChain= renderParams.commonRenderData.SwapChain;
	m_applicationRTVHeap = renderParams.RTVHeap;
	m_width= renderParams.commonRenderData.Width;
	m_height= renderParams.commonRenderData.Height;
	m_rtResourceFormat= renderParams.commonRenderData.RTResourceFormat;	
	m_psoManager= renderParams.commonRenderData.PSOMngr;
	m_frameResourceManager= renderParams.commonRenderData.FrameResourceMngr;
	m_texturesDescriptorHeap = renderParams.SRVHeap;
	m_finalRender.initialize(renderParams.commonRenderData);

	m_initialized = true;
}

void RenderManager::buildRenders()
{
	buildTechSRVs();

	//build Final Render	
	m_finalRender.set_DescriptorHeap_RTV(m_applicationRTVHeap);	
	m_finalRender.set_DescriptorHeap(m_texturesDescriptorHeap); // Textures SRV
	m_finalRender.build();
}

void RenderManager::draw(int flags)
{
	assert(m_initialized);

	int updatedFlags = flags;
	m_finalRender.draw(updatedFlags);
}

void RenderManager::buildTechSRVs()
{
	/*
		Add [TECHSRVCOUNT] 'NULL' SRVs for technical Textures (Results for other Renders) in the beggining of
		common DescriptorHeap (m_texturesDescriptorHeap). Later each Render will use his slot(s) and will create 'normal' SRV
	*/
	assert(m_texturesDescriptorHeap);

	UINT lSrvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE lhDescriptor(m_texturesDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	for (int i = 0; i < TECHSRVCOUNT; i++)
	{
		m_device->CreateShaderResourceView(nullptr,	&srvDesc, lhDescriptor);
		lhDescriptor.Offset(1, lSrvSize);
	}
}