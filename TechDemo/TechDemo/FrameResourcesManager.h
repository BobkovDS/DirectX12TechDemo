/*
	Stores and maintenances some count of FrameResources.
*/
#pragma once
//#include <vector>
//#include <d3d12.h>
//#include <wrl.h>
#include "UploadBuffer.h"
#include "ApplDataStructures.h"
#include <comdef.h>

// ------------ Interface Base class ----------------------
class IFrameResourcesManager
{
public:
	virtual ID3D12Resource* getCurrentObjectCBResource()=0;
	virtual ID3D12Resource* getCurrentPassCBResource() = 0;	
	virtual ID3D12Resource* getCurrentBoneCBResource() = 0;
	virtual ID3D12Resource* getCurrentSSAOCBResource() = 0;
	virtual ID3D12Resource* getCurrentDrawIDCBResource() = 0;
	virtual UINT getPassCBsize() = 0;
};

//--------------END of Interface Base class ---------------

template<class ConstObjectType, class PassConstsType,   class SSAOType>
class FrameResourcesManager: public IFrameResourcesManager
{	
	class FrameResource;
	std::vector<FrameResource*> m_frameResources;
	ID3D12Fence* m_fence;
	HANDLE m_eventHandle;
	UINT m_currentFR; // points to current FR. Function getFreeFR moves to next one
	bool m_initialized;
	//closed
	FrameResourcesManager(const FrameResourcesManager&) = delete;
	void operator=(const FrameResourcesManager&) = delete;	
public:

	typedef ConstObjectType constObjType;
	typedef PassConstsType passConsts;	
	typedef SSAOType ssaoType;	

	constObjType tmpConstObject;
	passConsts tmpPassConsts;
	ssaoType tmpPassSSAOConsts;

	class FrameResource /*FR*/ { 
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		std::unique_ptr<UploadBuffer<constObjType>> m_objectCB = nullptr; //constant buffer for every object
		std::unique_ptr<UploadBuffer<passConsts>> m_passCB = nullptr;// constant buffer for frame (pass)			
		std::unique_ptr<UploadBuffer<ssaoType>> m_SSAOCB = nullptr;
		std::unique_ptr<UploadBuffer<DirectX::XMFLOAT4X4>> m_bonesTransform = nullptr;
		std::unique_ptr<UploadBuffer<UINT>> m_drawInstancesID = nullptr;

		//closed
		FrameResource(const FrameResource&) = delete;
		void operator=(const FrameResource&) = delete;
	protected:
		friend FrameResourcesManager<constObjType, passConsts,  ssaoType>;
		UINT64 m_fenceValue = 0;
	public:		
		FrameResource(ID3D12Device* device, UINT constObjCount, UINT passCount, UINT SSAOCount, 
			UINT BoneTransformCount, UINT DrawIntancesCount);
		void setFenceValue(UINT64 newFenceValue) { m_fenceValue = newFenceValue; } 
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& getCommandAllocator() { return m_commandAllocator; }	
		UploadBuffer<constObjType>* getObjectCB() { return m_objectCB.get(); }
		UploadBuffer<passConsts>* getPassCB() { return m_passCB.get(); }		
		UploadBuffer<ssaoType>* getSSAOCB() { return m_SSAOCB.get(); }
		UploadBuffer<DirectX::XMFLOAT4X4>* getBoneCB() {	return m_bonesTransform.get();}
		UploadBuffer<UINT>* getDrawInstancesCB() {	return m_drawInstancesID.get();}
	};

	// device and fence will come from basic class BasicDXGI for Basic3D, 
	// constObjCount in current realisation is set in Basic3D constructor

	FrameResourcesManager();
	~FrameResourcesManager();

	int count() const { return m_frameResources.size(); }
	void Initialize(ID3D12Device* device, ID3D12Fence* fence,
		UINT constObjCount, UINT passCount, UINT SSAOCount, UINT BoneCount, UINT DrawIntancesCount);
	void getFreeFR(); // move a pointer to next FR and wait for when it becomes "clean"
	FrameResource* currentFR() const; // get current "clean" FR
	ID3D12Resource* getCurrentObjectCBResource();
	ID3D12Resource* getCurrentPassCBResource();	
	ID3D12Resource* getCurrentBoneCBResource();
	ID3D12Resource* getCurrentSSAOCBResource();
	ID3D12Resource* getCurrentDrawIDCBResource();
	UINT getPassCBsize();

	void changeCmdAllocator(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* pInitialState);
};


//------------------------------------- Declaration ------------------------------------

template<class ConstObjectType, class PassConstsType,   class SSAOType>
FrameResourcesManager<ConstObjectType, PassConstsType,  SSAOType>::FrameResourcesManager()
	:m_currentFR(0), m_initialized(false)
{
	m_eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
}

template<class ConstObjectType, class PassConstsType,   class SSAOType>
FrameResourcesManager<ConstObjectType, PassConstsType,  SSAOType>::~FrameResourcesManager()
{
	CloseHandle(m_eventHandle);

	for (int i = 0; i < m_frameResources.size(); i++)
	{
		delete m_frameResources.back();
		m_frameResources.pop_back();
	}
}

template<class ConstObjectType, class PassConstsType,   class SSAOType>
void FrameResourcesManager<ConstObjectType, PassConstsType,  SSAOType>
::Initialize(ID3D12Device* device, ID3D12Fence* fence, UINT constObjCount, UINT passCount, UINT SSAOCount, 
	UINT BoneCount, UINT DrawIntancesCount)
{
	if (m_initialized) return;

	assert(fence);
	assert(device);

	m_fence = fence;

	for (int i = 0; i < MAX_FRAMERESOURCE_COUNT; i++)
	{
		m_frameResources.push_back(new FrameResource(device, constObjCount, passCount, SSAOCount, BoneCount, DrawIntancesCount));
	}

	m_initialized = true;
}

template<class ConstObjectType, class PassConstsType,  class SSAOType>
void FrameResourcesManager<ConstObjectType, PassConstsType,  SSAOType>::getFreeFR()
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
		m_fence->SetEventOnCompletion(fenceValue, m_eventHandle);
		WaitForSingleObject(m_eventHandle, INFINITE);		
	}
};

template<class ConstObjectType, class PassConstsType,   class SSAOType>
typename FrameResourcesManager<ConstObjectType, PassConstsType,  SSAOType>::FrameResource*
FrameResourcesManager<ConstObjectType, PassConstsType,  SSAOType>::currentFR() const
{
	assert(m_initialized);
	return m_frameResources.at(m_currentFR);
}

template<class ConstObjectType, class PassConstsType,   class SSAOType>
ID3D12Resource* FrameResourcesManager<ConstObjectType, PassConstsType,  SSAOType>::getCurrentObjectCBResource()
{	
	return currentFR()->getObjectCB()->Resource();
}

template<class ConstObjectType, class PassConstsType,   class SSAOType>
ID3D12Resource* FrameResourcesManager<ConstObjectType, PassConstsType,  SSAOType>::getCurrentPassCBResource()
{	
	return currentFR()->getPassCB()->Resource();
}

template<class ConstObjectType, class PassConstsType,   class SSAOType>
ID3D12Resource* FrameResourcesManager<ConstObjectType, PassConstsType,  SSAOType>::getCurrentBoneCBResource()
{	
	return currentFR()->getBoneCB()->Resource();
}

template<class ConstObjectType, class PassConstsType,   class SSAOType>
ID3D12Resource* FrameResourcesManager<ConstObjectType, PassConstsType,  SSAOType>::getCurrentSSAOCBResource()
{	
	return currentFR()->getSSAOCB()->Resource();
}

template<class ConstObjectType, class PassConstsType, class SSAOType>
ID3D12Resource* FrameResourcesManager<ConstObjectType, PassConstsType, SSAOType>::getCurrentDrawIDCBResource()
{
	return currentFR()->getDrawInstancesCB()->Resource();
}

template<class ConstObjectType, class PassConstsType, class SSAOType>
UINT FrameResourcesManager<ConstObjectType, PassConstsType, SSAOType>::getPassCBsize()
{
	return Utilit3D::CalcConstantBufferByteSize(sizeof passConsts);
}

template<class ConstObjectType, class PassConstsType,   class SSAOType>
FrameResourcesManager<ConstObjectType, PassConstsType,  SSAOType>::FrameResource
::FrameResource(ID3D12Device* device, UINT constObjCount, UINT passCount, UINT SSAOCount, UINT BoneTransformCount, 
	UINT DrawIntancesCount)
{
	HRESULT res;

	m_fenceValue = 0;

	res = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(m_commandAllocator.GetAddressOf()));
	assert(SUCCEEDED(res));

	m_objectCB = std::make_unique<UploadBuffer<ConstObjectType>>
		(device, constObjCount, false); // <- false here because InstanceData goes how SRV
	m_passCB = std::make_unique<UploadBuffer<PassConstsType>>
		(device, passCount, true);
	
	m_SSAOCB = std::make_unique<UploadBuffer<ssaoType>>(device, SSAOCount, true);
	m_bonesTransform= std::make_unique<UploadBuffer<DirectX::XMFLOAT4X4>>(device, BoneTransformCount, false);
	m_drawInstancesID= std::make_unique<UploadBuffer<UINT>>(device, DrawIntancesCount, false); // TO_DO: Delete this

}

template<class ConstObjectType, class PassConstsType,   class SSAOType>
void FrameResourcesManager<ConstObjectType, PassConstsType,  SSAOType>::changeCmdAllocator(
	ID3D12GraphicsCommandList* cmdList, 
	ID3D12PipelineState* pInitialState)
{	
	auto currentCmdAllocator = currentFR()->getCommandAllocator();

	HRESULT res;
	res = currentCmdAllocator->Reset();
	assert(SUCCEEDED(res));

	res = cmdList->Reset(currentCmdAllocator.Get(), pInitialState);
	//_com_error err(res);
	//std::wstring errMsg = err.ErrorMessage();

	assert(SUCCEEDED(res));
}