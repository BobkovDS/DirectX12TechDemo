#include "TechDemo.h"

#define PASSCONSTBUFCOUNTDCM 6
#define PASSCONSTBUFCOUNT 1/*Frame*/ + 0/*Shadow*/ +0/*Mirror*/ + PASSCONSTBUFCOUNTDCM
#define SSAOCONSTBUFCOUNT 1

using namespace DirectX;

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

	m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager, &m_skeletonManager);
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
	update_camera();	
	update_objectCB();
	update_BoneData();
	update_passCB();
}

void TechDemo::update_camera()
{

}

void TechDemo::update_objectCB()
{

}

void TechDemo::update_BoneData()
{
	auto currBoneCB = m_frameResourceManager.currentFR()->getBoneCB();
	float t = 0;
	float lBoneCBID= 0;

	int lSkeletonCount = m_skeletonManager.getSkeletonCount();
	for (int si = 0; si < lSkeletonCount; si++)
	{
		SkinnedData& lSkeleton = m_skeletonManager.getSkeleton(si);
		const std::vector<DirectX::XMFLOAT4X4>& lFinalMatrices = lSkeleton.getFinalTransforms(t, 0);

		for (int i = 0; i < lFinalMatrices.size(); i++)
		{
			currBoneCB->CopyData(lBoneCBID++, lFinalMatrices[i]);
		}
	}
}

void TechDemo::update_passCB()
{
	auto mMainPassCB = m_frameResourceManager.tmpPassConsts;

	XMMATRIX view = m_camera.getView();
	XMMATRIX proj = m_camera.lens->getProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	//XMStoreFloat4x4(&mMainPassCB.ReflectWord, XMMatrixTranspose(perpectViewProj)); // Reflection here is used for Projector Proj matrix

	mMainPassCB.RenderTargetSize = XMFLOAT2((float)width(), (float)height());
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / width(), 1.0f / height());
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.EyePosW = m_camera.getPosition3f();
	mMainPassCB.TotalTime = m_animationTimer.totalTime();
	mMainPassCB.doesUseMirrorLight = 0; // should we use Mirror light?

	const std::vector<CPULight>& lights = m_scene.getLights();

	for (size_t i = 0; i < lights.size(); i++)
	{
		mMainPassCB.Lights[i].Direction = lights.at(i).Direction;
		mMainPassCB.Lights[i].Strength = lights.at(i).Strength;
		mMainPassCB.Lights[i].Position = lights.at(i).Position;
		mMainPassCB.Lights[i].spotPower = lights.at(i).spotPower;
		mMainPassCB.Lights[i].falloffStart = lights.at(i).falloffStart;
		mMainPassCB.Lights[i].falloffEnd = lights.at(i).falloffEnd;
		mMainPassCB.Lights[i].lightType = lights.at(i).lightType + 1; // 0 - is undefined type of light
		mMainPassCB.Lights[i].turnOn = lights.at(i).turnOn;		
	}
	
	auto currPassCB = m_frameResourceManager.currentFR()->getPassCB();
	currPassCB->CopyData(0, mMainPassCB);	
}

void TechDemo::build_defaultCamera()
{
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(-1.0, 3.0f, -13.0f, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_camera.lookAt(pos, target, up);
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

void TechDemo::onReSize(int newWidth, int newHeight)
{
	BasicDXGI::onReSize(newWidth, newHeight);

	float w = static_cast<float>(width());
	float h = static_cast<float>(height());
	float aspect = w / h;

	m_camera.lens->setLens(0.25f*XM_PI, aspect, 1.0f, 100.0f);
	m_camera.buildFrustumBounding();
}