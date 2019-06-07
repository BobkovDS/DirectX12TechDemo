#include "RenderManager.h"

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

	m_finalRender.initialize(renderParams.commonRenderData);

	m_initialized = true;
}

void RenderManager::buildRenders()
{
	//build Final Render
	m_finalRender.set_DescriptorHeap_RTV(m_applicationRTVHeap);
	m_finalRender.build();
}

void RenderManager::draw(int flags)
{
	assert(m_initialized);

	int updatedFlags = flags;
	m_finalRender.draw(updatedFlags);
}