#include "TechDemo.h"



TechDemo::TechDemo(HINSTANCE hInstance, const std::wstring& applName, int width, int height)
	:BasicDXGI(hInstance, applName, width, height)	
{
	//Utilit3D::nullitizator();
}


TechDemo::~TechDemo()
{
}

void TechDemo::init3D()
{
	BasicDXGI::init3D();
		
	Utilit3D::initialize(m_device.Get(), m_cmdList.Get());

	m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager);
	m_fbx_loader.loadSceneFile("Models\\Wolf.fbx");

	m_resourceManager.loadTexture();

	//here we come with closed m_cmdList
	HRESULT res;

	// create_DSV has closed m_cmdList, lets open it again
	res = m_cmdList->Reset(m_cmdAllocator.Get(), nullptr);
	assert(SUCCEEDED(res));
	
	RenderManagerMessanger lRenderManagerParams;
	lRenderManagerParams.RTVHeap = m_rtvHeap.Get();
	lRenderManagerParams.commonRenderData.Device = m_device.Get();
	lRenderManagerParams.commonRenderData.CmdList= m_cmdList.Get();
	lRenderManagerParams.commonRenderData.SwapChain = m_swapChain.Get();
	lRenderManagerParams.commonRenderData.RTResourceFormat = backBufferFormat();
	lRenderManagerParams.commonRenderData.Width = width();
	lRenderManagerParams.commonRenderData.Height = height();
	lRenderManagerParams.commonRenderData.Scene = &m_scene;
	lRenderManagerParams.commonRenderData.FrameResourceMngr = &m_frameResourceManager;
	

	m_renderManager.initialize(lRenderManagerParams);

	m_renderManager.draw(1);
	m_cmdList->Close();
	ID3D12CommandList* cmdsList[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, cmdsList);

	FlushCommandQueue();
}