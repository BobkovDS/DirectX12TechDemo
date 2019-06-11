#include "TechDemo.h"

#define PASSCONSTBUFCOUNTDCM 6
#define PASSCONSTBUFCOUNT 1/*Frame*/ + 0/*Shadow*/ +0/*Mirror*/ + PASSCONSTBUFCOUNTDCM
#define SSAOCONSTBUFCOUNT 1

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
	//here we come with closed m_cmdList
	HRESULT res;

	// create_DSV has closed m_cmdList, lets open it again
	res = m_cmdList->Reset(m_cmdAllocator.Get(), nullptr);
	assert(SUCCEEDED(res));
	
	Utilit3D::initialize(m_device.Get(), m_cmdList.Get());

	m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager);
	m_fbx_loader.loadSceneFile("Models\\Wolf.fbx");

	m_resourceManager.loadTexture();	

	RenderManagerMessanger lRenderManagerParams;
	lRenderManagerParams.RTVHeap = m_rtvHeap.Get();
	lRenderManagerParams.commonRenderData.Device = m_device.Get();
	lRenderManagerParams.commonRenderData.CmdList= m_cmdList.Get();
	lRenderManagerParams.commonRenderData.SwapChain = m_swapChain.Get();	
	lRenderManagerParams.commonRenderData.RTResourceFormat = backBufferFormat();
	lRenderManagerParams.commonRenderData.DSResourceFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	lRenderManagerParams.commonRenderData.Width = width();
	lRenderManagerParams.commonRenderData.Height = height();
	lRenderManagerParams.commonRenderData.Scene = &m_scene;
	lRenderManagerParams.commonRenderData.FrameResourceMngr = &m_frameResourceManager;
	lRenderManagerParams.SRVHeap = m_resourceManager.getTexturesSRVDescriptorHeap();

	int lBonesCount = 1;
	m_frameResourceManager.Initialize(m_device.Get(), m_fence.Get(), m_objectManager.getCommonInstancesCount(),
		PASSCONSTBUFCOUNT, SSAOCONSTBUFCOUNT, lBonesCount);
	
	m_scene.build(&m_objectManager);
	m_renderManager.initialize(lRenderManagerParams);
	m_renderManager.buildRenders();	
	

	m_cmdList->Close();
	ID3D12CommandList* cmdsList[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, cmdsList);

	FlushCommandQueue();
}

void TechDemo::update()
{
	m_scene.update();
	update_BoneData();
}

void TechDemo::update_BoneData()
{
	auto currBoneCB = m_frameResourceManager.currentFR()->getBoneCB();

	float t = 0;

	const std::vector<DirectX::XMFLOAT4X4>& lFinalMatrices = m_skinnedData.getFinalTransforms(t,0);
		
	for (int i = 0; i < lFinalMatrices.size(); i++)
	{
		currBoneCB->CopyData(i, lFinalMatrices[i]);
	}
}

void TechDemo::work()
{
	// here we begin new Graphic Frame
	m_frameResourceManager.getFreeFR(); // so we need new Frame resource

	// for graphic frame we use CommandAllocator from FR
	m_frameResourceManager.changeCmdAllocator(m_cmdList.Get(), nullptr);

	// update data
	update();

	// draw data
	m_renderManager.draw(1);


	m_cmdList->Close();
	ID3D12CommandList* CmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, CmdLists);

	m_swapChain->Present(0, 0);
	m_frameResourceManager.currentFR()->setFenceValue(getFenceValue());
	setFence();
}