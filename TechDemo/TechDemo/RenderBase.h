#pragma once
#include "GraphicDataStructures.h"
#include "Scene.h"
#include "FrameResourcesManager.h"
#include "ResourceManager.h"

#define TECHSRVCOUNT 11

#define TECHSLOT_SKY 0
#define TECHSLOT_DCM 1
#define TECHSLOT_RNDVECTORMAP 2 // Random Vector Map
#define TECHSLOT_SSAO_VSN 3 // ViewSpace Normal Map
#define TECHSLOT_SSAO_DPT 4// Depth Map
#define TECHSLOT_SSAO_AO 5// AO Map
#define TECHSLOT_BLUR_A_SRV 6// Blur Resource A: SRV
#define TECHSLOT_BLUR_B_UAV 7// Blur Resource B: UAV
#define TECHSLOT_BLUR_B_SRV 8// Blur Resource B: SRV
#define TECHSLOT_BLUR_A_UAV 9// Blur Resource A: UAV
#define TECHSLOT_SHADOW 10// Shadow Map 

struct RenderResource {
	void createResource(ID3D12Device* device, DXGI_FORMAT resourceFormat, D3D12_RESOURCE_FLAGS resourceFlags,
		UINT width, UINT height, D3D12_CLEAR_VALUE* optClear = NULL, UINT16 texturesCount=1);

	void resize(UINT width, UINT height);	

	void changeState(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
	{
		cmdList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(m_resource.Get(),stateBefore, stateAfter));
	}
	const D3D12_RESOURCE_BARRIER getBarrier(D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
	{
		return CD3DX12_RESOURCE_BARRIER::Transition(m_resource.Get(), stateBefore, stateAfter);
	}

	ID3D12Resource* getResource();	
	~RenderResource();
private:
	ID3D12Device* m_device;
	DXGI_FORMAT m_resourceFormat;
	D3D12_RESOURCE_FLAGS m_resourceFlags;	
	D3D12_CLEAR_VALUE* m_optClear;
	UINT16 m_texturesCount;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
};

struct RenderMessager {
	ID3D12Device* Device;
	ID3D12GraphicsCommandList* CmdList;
	IDXGISwapChain3* SwapChain;		
	DXGI_FORMAT DSResourceFormat; //DXGI_FORMAT_D24_UNORM_S8_UINT
	DXGI_FORMAT RTResourceFormat;
	UINT Width;
	UINT Height;
	Scene* Scene;	
	IFrameResourcesManager* FrameResourceMngr;
	ResourceManager* ResourceMngr;
};

class RenderBase
{

protected:
	ID3D12Device* m_device;
	ID3D12GraphicsCommandList* m_cmdList;
	Scene* m_scene;	
	IFrameResourcesManager* m_frameResourceManager;
	ResourceManager* m_resourceManager;

	UINT m_width;
	UINT m_height;
	bool m_initialized;
	bool m_dsResourceWasSetBefore; // The resource(s) can be only created by Render or Set from another source. No combinated variants.
	bool m_rtResourceWasSetBefore;
	bool m_dsvHeapWasSetBefore; // The heap(s) can be only created by Render or Set from another source. No combinated variants.
	bool m_rtvHeapWasSetBefore;

	DXGI_FORMAT m_dsResourceFormat;
	DXGI_FORMAT m_rtResourceFormat;
	D3D12_VIEWPORT m_viewPort;
	D3D12_RECT m_scissorRect;

	UINT m_rtvDescriptorSize;	
	UINT m_currentResourceID;

	std::vector<RenderResource> m_own_resources;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_own_dsvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_own_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_own_srv_uav_Heap; 

	// "Resource interface"
	RenderResource* m_dsResource;
	std::vector<RenderResource*> m_rtResources;
	std::vector<RenderResource*> m_uavResources;
	ID3D12DescriptorHeap* m_descriptorHeap;
	ID3D12DescriptorHeap* m_dsvHeap;
	ID3D12DescriptorHeap* m_rtvHeap;	
	D3D12_GPU_DESCRIPTOR_HANDLE m_techSRVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_textureSRVHandle;	

public:
	RenderResource* create_Resource(DXGI_FORMAT resourceFormat, D3D12_RESOURCE_FLAGS resourceFlags,
		UINT width = 0, UINT height = 0,
		D3D12_CLEAR_VALUE* optClear = NULL,
		UINT16 texturesCount = 1);

	void create_Resource_DS(DXGI_FORMAT resourceFormat);
	void create_Resource_RT(DXGI_FORMAT resourceFormat, UINT width = 0, UINT height = 0, UINT16 resourcesCount =1);
	void create_DescriptorHeap_DSV();
	void create_DescriptorHeap_RTV(UINT descriptorsCount);
	void create_DSV(DXGI_FORMAT viewFormat = DXGI_FORMAT_UNKNOWN);
	void create_RTV(DXGI_FORMAT viewFormat = DXGI_FORMAT_UNKNOWN);
	//void set_Resource_DS(ID3D12Resource* Resource); //TO_DO: delete
	//void set_Resource_RT(ID3D12Resource* Resource); //TO_DO: delete
	void set_DescriptorHeap_RTV(ID3D12DescriptorHeap* rtvHeap); // set Descriptor Heap for RT view
	void set_DescriptorHeap_DSV(ID3D12DescriptorHeap* dsvHeap); // set Descriptor Heap for RT view

	void set_DescriptorHeap(ID3D12DescriptorHeap* srvDescriptorHeap); // set Descriptor Heap for Textures SRVs

	ID3D12Resource* get_Resource(int resourceIndex);
	ID3D12Resource* get_Resource_DS();
	ID3D12Resource* get_Resource_RT();
	int get_ResourceCount();
	ID3D12DescriptorHeap* get_rtvHeapPointer();
	ID3D12DescriptorHeap* get_dsvHeapPointer();	

	void initialize(const RenderMessager& renderParams); // initialize the Render
	void build(); // Do any other operations to build Render (to add more resources, create SRVs and etc)
	void draw(int flags);
	void resize(UINT newWidth, UINT newHeight);	
	RenderBase();
	virtual ~RenderBase();
};

