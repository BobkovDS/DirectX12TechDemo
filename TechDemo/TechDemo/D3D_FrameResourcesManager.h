/*
	Stores and maintenances some count of FrameResources.
*/
#pragma once
//#include <vector>
//#include <d3d12.h>
//#include <wrl.h>
#include "UploadBuffer.h"
#include "DataStructures.h"

template<class ConstObjectType, class PassConstsType, class MaterialType, class SSAOType>
class D3D_FrameResourcesManager
{	
	class FrameResource;
	std::vector<FrameResource*> m_frameResources;
	ID3D12Fence* m_fence;
	UINT m_currentFR; // points to current FR. Function getFreeFR moves to next one
	bool m_initialized;
	//closed
	D3D_FrameResourcesManager(const D3D_FrameResourcesManager&) = delete;
	void operator=(const D3D_FrameResourcesManager&) = delete;	
public:

	typedef ConstObjectType constObjType;
	typedef PassConstsType passConsts;
	typedef MaterialType materialType;
	typedef SSAOType ssaoType;
	constObjType tmpConstObject;
	passConsts tmpPassConsts;
	ssaoType tmpPassSSAOConsts;

	class FrameResource /*FR*/ { 
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		std::unique_ptr<UploadBuffer<constObjType>> m_objectCB = nullptr; //constant buffer for every object
		std::unique_ptr<UploadBuffer<passConsts>> m_passCB = nullptr;// constant buffer for frame (pass)	
		std::unique_ptr<UploadBuffer<materialType>> m_materialCB = nullptr;
		std::unique_ptr<UploadBuffer<ssaoType>> m_SSAOCB = nullptr;
		std::unique_ptr<UploadBuffer<DirectX::XMFLOAT4X4>> m_bonesTransform = nullptr;

		//closed
		FrameResource(const FrameResource&) = delete;
		void operator=(const FrameResource&) = delete;
	protected:
		friend D3D_FrameResourcesManager<constObjType, passConsts, materialType, ssaoType>;
		UINT64 m_fenceValue = 0;
	public:		
		FrameResource(ID3D12Device* device, UINT constObjCount, UINT passCount, UINT materialCount, 
			UINT SSAOCount, UINT BoneTransformCount);
		void setFenceValue(UINT64 newFenceValue) { m_fenceValue = newFenceValue; } 
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& getCommandAllocator() { return m_commandAllocator; }	
		UploadBuffer<constObjType>* getObjectCB() { return m_objectCB.get(); }
		UploadBuffer<passConsts>* getPassCB() { return m_passCB.get(); }
		UploadBuffer<materialType>* getMaterialCB() { return m_materialCB.get(); }
		UploadBuffer<ssaoType>* getSSAOB() { return m_SSAOCB.get(); }
		UploadBuffer<DirectX::XMFLOAT4X4>* getBoneCB() {	return m_bonesTransform.get();}
	};

	// device and fence will come from basic class BasicDXGI for Basic3D, 
	// constObjCount in current realisation is set in Basic3D constructor

	D3D_FrameResourcesManager();
	~D3D_FrameResourcesManager();

	int count() const { return m_frameResources.size(); }
	void Initialize(ID3D12Device* device, ID3D12Fence* fence,
		UINT constObjCount, UINT passCount, UINT materialCount, UINT SSAOCount, UINT BoneCount);
	void getFreeFR(); // move a pointer to next FR and wait for when it becomes "clean"
	FrameResource* currentFR() const; // get current "clean" FR
	void changeCmdAllocator(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* pInitialState);
};


//------------------------------------- Declaration ------------------------------------

template<class ConstObjectType, class PassConstsType, class MaterialType, class SSAOType>
D3D_FrameResourcesManager<ConstObjectType, PassConstsType, MaterialType, SSAOType>::D3D_FrameResourcesManager()
	:m_currentFR(0), m_initialized(false)
{
}

template<class ConstObjectType, class PassConstsType, class MaterialType, class SSAOType>
D3D_FrameResourcesManager<ConstObjectType, PassConstsType, MaterialType, SSAOType>::~D3D_FrameResourcesManager()
{
	for (int i = 0; i < m_frameResources.size(); i++)
	{
		delete m_frameResources.back();
		m_frameResources.pop_back();
	}
}

template<class ConstObjectType, class PassConstsType, class MaterialType, class SSAOType>
void D3D_FrameResourcesManager<ConstObjectType, PassConstsType, MaterialType, SSAOType>
::Initialize(ID3D12Device* device, ID3D12Fence* fence, UINT constObjCount, UINT passCount, UINT materialCount, UINT SSAOCount, UINT BoneCount)
{
	if (m_initialized) return;

	assert(fence);
	assert(device);

	m_fence = fence;

	for (int i = 0; i < MAX_FRAMERESOURCE_COUNT; i++)
	{
		m_frameResources.push_back(new FrameResource(device, constObjCount, passCount, materialCount, SSAOCount, BoneCount));
	}

	m_initialized = true;
}

template<class ConstObjectType, class PassConstsType,class MaterialType, class SSAOType>
void D3D_FrameResourcesManager<ConstObjectType, PassConstsType, MaterialType, SSAOType>::getFreeFR()
{
	if (!m_initialized) return;

	/*
	1) Move the pointer to the next FrameResource N
	2) N FR is dirty and used. Does GPU completed commands for this FR?
	3) If YES go to the step 4, if NOT, wait when GPU has proccessed this FR
	4) Now we think that this FR is clean and ready to be used in a current graphic frame.
	Method currentFR() provides access to this "clean" FR.
	*/
	m_currentFR = (m_currentFR + 1) % m_frameResources.size();
	UINT64 fenceValue = m_frameResources.at(m_currentFR)->m_fenceValue;

	if (
		(fenceValue != 0) &&
		(m_fence->GetCompletedValue() < fenceValue)
		)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		m_fence->SetEventOnCompletion(fenceValue, eventHandle);
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
};

template<class ConstObjectType, class PassConstsType, class MaterialType, class SSAOType>
typename D3D_FrameResourcesManager<ConstObjectType, PassConstsType, MaterialType, SSAOType>::FrameResource*
D3D_FrameResourcesManager<ConstObjectType, PassConstsType, MaterialType, SSAOType>::currentFR() const
{
	if (!m_initialized) return nullptr ;
	return m_frameResources.at(m_currentFR);
}

template<class ConstObjectType, class PassConstsType, class MaterialType, class SSAOType>
D3D_FrameResourcesManager<ConstObjectType, PassConstsType, MaterialType, SSAOType>::FrameResource
::FrameResource(ID3D12Device* device, UINT constObjCount, UINT passCount, UINT materialCount, UINT SSAOCount, UINT BoneTransformCount)
{
	HRESULT res;

	res = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(m_commandAllocator.GetAddressOf()));
	assert(SUCCEEDED(res));

	m_objectCB = std::make_unique<UploadBuffer<ConstObjectType>>
		(device, constObjCount, false); // <- false here because InstanceData goes how SRV
	m_passCB = std::make_unique<UploadBuffer<PassConstsType>>
		(device, passCount, true);
	//m_materialCB = std::make_unique<UploadBuffer<MaterialType>>	(device, materialCount, true);
	m_SSAOCB = std::make_unique<UploadBuffer<ssaoType>>(device, SSAOCount, true);
	m_bonesTransform= std::make_unique<UploadBuffer<DirectX::XMFLOAT4X4>>(device, BoneTransformCount, false);

}

template<class ConstObjectType, class PassConstsType, class MaterialType, class SSAOType>
void D3D_FrameResourcesManager<ConstObjectType, PassConstsType, MaterialType, SSAOType>::changeCmdAllocator(
	ID3D12GraphicsCommandList* cmdList, 
	ID3D12PipelineState* pInitialState)
{	
	auto currentCmdAllocator = currentFR()->getCommandAllocator();

	HRESULT res;
	res = currentCmdAllocator->Reset();
	assert(SUCCEEDED(res));

	res = cmdList->Reset(currentCmdAllocator.Get(), pInitialState);
	assert(SUCCEEDED(res));
}