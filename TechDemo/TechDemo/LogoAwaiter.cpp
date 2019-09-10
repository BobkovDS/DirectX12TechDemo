#include "LogoAwaiter.h"

using namespace DirectX;

LogoAwaiter::LogoAwaiter(HINSTANCE hInstance, const std::wstring& applName, int width, int height)
	:BasicDXGI(hInstance, applName, width, height)
{
	m_init3D_done = false;
}


LogoAwaiter::~LogoAwaiter()
{
}


void LogoAwaiter::init3D()
{
	BasicDXGI::init3D();

	Utilit3D::initialize(m_device.Get(), m_cmdList.Get());
	m_frameResourceManager.Initialize(m_device.Get(), m_fence.Get(), 1,	PASSCONSTBUFCOUNT, 0, 0, 1);

	m_psoLayer.buildPSO(m_device.Get(), backBufferFormat(), DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
	createLogoInformation();

}

void LogoAwaiter::work()
{
	FlushCommandQueue();

	m_frameResourceManager.getFreeFR();

	// for graphic frame we use CommandAllocator from FR
	m_frameResourceManager.changeCmdAllocator(m_cmdList.Get(), nullptr);

	// update data
	//update();
	
	// draw data
	draw();

	m_cmdList->Close();
	ID3D12CommandList* CmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, CmdLists);

#ifdef GUI_HUD
	renderUI();
#endif

	m_swapChain->Present(0, 0);
	m_frameResourceManager.currentFR()->setFenceValue(getFenceValue());
	FlushCommandQueue();
}

void LogoAwaiter::draw()
{

}

void LogoAwaiter::onReSize(int newWidth, int newHeight)
{
	BasicDXGI::onReSize(newWidth, newHeight);
	// Here we have cmdList is closed 

	if (m_camera)
	{
		float w = static_cast<float>(width());
		float h = static_cast<float>(height());
		float aspect = w / h;

		PerspectiveCameraLens* lPerspectiveLens = dynamic_cast<PerspectiveCameraLens*> (m_camera->lens);
		lPerspectiveLens->setAspectRatio(aspect);
		m_camera->buildFrustumBounding();
	}

	if (!m_init3D_done) return; // onResize call it before how 3D part was init

	/*HRESULT res = 0;
	res = m_cmdList->Reset(m_cmdAllocator.Get(), nullptr);
	assert(SUCCEEDED(res));	

	res = m_cmdList->Close();
	ID3D12CommandList* cmdsList[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, cmdsList);
	FlushCommandQueue();*/
}

void LogoAwaiter::createLogoInformation()
{
	std::vector<VertexGPU> lVertices;
	std::vector<uint32_t> lIndices;

	VertexGPU lV1 = {};	
	lV1.Pos = XMFLOAT3(-1.0f, 0.0f, 0.0f);
	lV1.Normal = XMFLOAT3(1.0f, 0.0f, 0.0f);

	lVertices.push_back(lV1);
	lV1.Pos = XMFLOAT3(1.0f, 0.0f, 0.0f);
	lVertices.push_back(lV1);

	lIndices.push_back(0);
	lIndices.push_back(1);

	UINT32 lVertexSizeInByte = sizeof(VertexGPU);
	m_logoMesh = std::make_unique<Mesh>();	
	m_logoMesh->VertexByteStride = lVertexSizeInByte;
	m_logoMesh->VertexBufferByteSize = lVertexSizeInByte * lVertices.size();	
	m_logoMesh->IndexBufferByteSize = sizeof(uint32_t) * lIndices.size();
	m_logoMesh->IndexFormat = DXGI_FORMAT_R32_UINT;

	SubMesh lsubMesh = {};
	lsubMesh.VertextCount = lVertices.size();
	lsubMesh.IndexCount = lIndices.size();	

	Utilit3D::UploadMeshToDefaultBuffer<Mesh, VertexGPU, uint32_t>(m_logoMesh.get(), lVertices, lIndices);
}