#include "TechDemo.h"



TechDemo::TechDemo(HINSTANCE hInstance, const std::wstring& applName, int width, int height)
	:BasicDXGI(hInstance, applName, width, height)
	
{
}


TechDemo::~TechDemo()
{
}

void TechDemo::init3D()
{
	BasicDXGI::init3D();

	m_utilit3D.initialize(m_device.Get(), m_cmdList.Get());
	m_fbx_loader.Initialize(&m_objectManager, &m_resourceManager, &m_utilit3D);
	m_fbx_loader.loadSceneFile("Models\\Wolf.fbx");

	//here we come with closed m_cmdList
	HRESULT res;

	// create_DSV has closed m_cmdList, lets open it again
	res = m_cmdList->Reset(m_cmdAllocator.Get(), nullptr);
	assert(SUCCEEDED(res));
		   
	m_cmdList->Close();
	ID3D12CommandList* cmdsList[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, cmdsList);

	FlushCommandQueue();
}