#include "TechDemo.h"

#define PASSCONSTBUFCOUNTDCM 6
#define PASSCONSTBUFCOUNT 1/*Frame*/ + 0/*Shadow*/ +0/*Mirror*/ + PASSCONSTBUFCOUNTDCM
#define SSAOCONSTBUFCOUNT 1

using namespace DirectX;

TechDemo::TechDemo(HINSTANCE hInstance, const std::wstring& applName, int width, int height)
	:BasicDXGI(hInstance, applName, width, height)	
{
	m_init3D_done = false;
	//Utilit3D::nullitizator();
}
TechDemo::~TechDemo()
{
}

void TechDemo::onMouseDown(WPARAM btnState, int x, int y)
{
	m_mouseDown = true;
	m_mouseDownPoint.x = x;
	m_mouseDownPoint.y = y;	
}

void TechDemo::onMouseUp(WPARAM btnState, int x, int y)
{
	m_mouseDown = false;
}

void TechDemo::onMouseMove(WPARAM btnState, int x, int y)
{
	if (m_mouseDown)
	{

		//if CTRL
		if (btnState == (MK_LBUTTON | MK_CONTROL))
		{
			//if (y != m_MouseDownPoint.y) mTheta += (y - m_MouseDownPoint.y)/abs(y - m_MouseDownPoint.y) *0.5f;

			//if (x != m_MouseDownPoint.x) mPhi += (m_MouseDownPoint.x - x)/abs(m_MouseDownPoint.x - x)* 0.5f;

		}
		else if (btnState == (MK_RBUTTON | MK_CONTROL))
		{
			//if (x != m_MouseDownPoint.x) mRadius += (m_MouseDownPoint.x - x)/abs(m_MouseDownPoint.x - x)* 1.0f;
		}

		else if (btnState == MK_LBUTTON)
		{
			float dx = XMConvertToRadians(0.25f*static_cast<float>(x - m_mouseDownPoint.x));
			float dy = XMConvertToRadians(0.25f*static_cast<float>(y - m_mouseDownPoint.y));

			m_camera.pitch(dy);
			m_camera.rotateY(dx);
		}
		m_mouseDownPoint.x = x;
		m_mouseDownPoint.y = y;
	}
}

void TechDemo::onKeyDown(WPARAM btnState)
{
	// other keyspressing

	if (GetAsyncKeyState('0') & 0x8000)
		toLimitFPS = !toLimitFPS;	

	if (btnState == VK_SPACE)
		m_animationTimer.tt_Pause();
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


	//// Load Wolf with Animation
	//{
	//	FBXFileLoader m_fbx_loader;
	//	m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager, &m_skeletonManager);
	//	m_fbx_loader.loadSceneFile("Models\\Wolf.fbx");
	//}

	////Load a house
	//{
	//	FBXFileLoader m_fbx_loader;
	//	m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager, &m_skeletonManager);
	//	m_fbx_loader.loadSceneFile("Models\\Cottage.fbx");
	//}
	//	

	// Load a Tree
	{
		FBXFileLoader m_fbx_loader;
		m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager, &m_skeletonManager);
		m_fbx_loader.loadSceneFile("Models\\Tree1.fbx");
	}

	m_skeletonManager.evaluateAnimationsTime();
	m_resourceManager.loadTexture();	
	m_resourceManager.loadMaterials();

	RenderManagerMessanger lRenderManagerParams;
	lRenderManagerParams.RTVHeap = m_rtvHeap.Get();
	lRenderManagerParams.RTResources = m_swapChainBuffers;
	lRenderManagerParams.commonRenderData.Device = m_device.Get();
	lRenderManagerParams.commonRenderData.CmdList= m_cmdList.Get();
	lRenderManagerParams.commonRenderData.SwapChain = m_swapChain.Get();	
	lRenderManagerParams.commonRenderData.RTResourceFormat = backBufferFormat();
	lRenderManagerParams.commonRenderData.DSResourceFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	lRenderManagerParams.commonRenderData.Width = width();
	lRenderManagerParams.commonRenderData.Height = height();
	lRenderManagerParams.commonRenderData.Scene = &m_scene;
	lRenderManagerParams.commonRenderData.FrameResourceMngr = &m_frameResourceManager;
	lRenderManagerParams.commonRenderData.ResourceMngr = &m_resourceManager;
	lRenderManagerParams.SRVHeap = m_resourceManager.getTexturesSRVDescriptorHeap();

	int lBonesCount = 100;
	m_frameResourceManager.Initialize(m_device.Get(), m_fence.Get(), m_objectManager.getCommonInstancesCount(),
		PASSCONSTBUFCOUNT, SSAOCONSTBUFCOUNT, lBonesCount);
	
	m_scene.build(&m_objectManager);
	m_renderManager.initialize(lRenderManagerParams);
	m_renderManager.buildRenders();	
	

	m_cmdList->Close();
	ID3D12CommandList* cmdsList[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, cmdsList);

	FlushCommandQueue();
	m_init3D_done = true;

	m_animationTimer.tt_RunStop();

	build_defaultCamera();
}

void TechDemo::update()
{
	m_animationTimer.tick();
	m_scene.update();
	update_camera();	
	update_objectCB();
	update_BoneData();
	update_passCB();
}

void TechDemo::update_camera()
{
	float dt = 0.005f;
	float da = XMConvertToRadians(0.1f);

	if (toLimitFPS)
	{
		dt *= 10.0f;
		da = XMConvertToRadians(0.1f*10.0f);
	}

	if (GetAsyncKeyState('W') & 0x8000)
		m_camera.walk(dt);

	if (GetAsyncKeyState('S') & 0x8000)
		m_camera.walk(-dt);

	if (GetAsyncKeyState('A') & 0x8000)
		m_camera.strafe(-dt);

	if (GetAsyncKeyState('D') & 0x8000)
		m_camera.strafe(dt);

	m_camera.updateViewMatrix();
}

void TechDemo::update_objectCB()
{
	auto lInstances = m_scene.getInstances();

	if (lInstances.size() == 0) return;

	auto currCBObject = m_frameResourceManager.currentFR()->getObjectCB();

	InstanceDataGPU lInst;

	lInst = *lInstances[0];

	for (int i=0; i< lInstances.size(); i++)
		currCBObject->CopyData(i, *lInstances[i]);
		//0currCBObject->CopyData(i, lInst);
}

void TechDemo::update_BoneData()
{
	
	float lTickTime = m_animationTimer.deltaTime();

	//m_animTime = 0;
	auto currBoneCB = m_frameResourceManager.currentFR()->getBoneCB();
	float lBoneCBID= 0;

	int lSkeletonCount = m_skeletonManager.getSkeletonCount();
	for (int si = 0; si < lSkeletonCount; si++)
	{
		SkinnedData& lSkeleton = m_skeletonManager.getSkeleton(si);
		const std::vector<DirectX::XMFLOAT4X4>& lFinalMatrices = lSkeleton.getFinalTransforms(lTickTime, 0);

		InstanceDataGPU ltmp;
		for (int i = 0; i < lFinalMatrices.size(); i++)
		{
			currBoneCB->CopyData(lBoneCBID++, lFinalMatrices[i]);
			//currBoneCB->CopyData(lBoneCBID++, ltmp.World);
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
	// Here we have cmdList is closed 

	float w = static_cast<float>(width());
	float h = static_cast<float>(height());
	float aspect = w / h;

	m_camera.lens->setLens(0.25f*XM_PI, aspect, 1.0f, 100.0f);
	m_camera.buildFrustumBounding();

	if (!m_init3D_done) return; // onResize call it before how 3D part was init

	HRESULT res = 0;
	res = m_cmdList->Reset(m_cmdAllocator.Get(), nullptr);
	assert(SUCCEEDED(res));
	
	m_renderManager.resize(newWidth, newHeight);
	
	res = m_cmdList->Close();
	ID3D12CommandList* cmdsList[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, cmdsList);
	FlushCommandQueue();
}