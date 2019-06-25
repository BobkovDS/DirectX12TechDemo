#pragma once
#include "GraphicDataStructures.h"
#include "Scene.h"
#include "PSOManager.h"
#include "FrameResourcesManager.h"
#include "ResourceManager.h"

#define TECHSRVCOUNT 10

#define TECHSLOT_SKY 0

struct RenderResource {
	void createResource(ID3D12Device* device, DXGI_FORMAT resourceFormat, UINT width, UINT height, D3D12_CLEAR_VALUE* optClear = NULL);
	void resize(UINT width, UINT height);
	ID3D12Resource* getResource();	
	~RenderResource();
private:
	ID3D12Device* m_device;
	DXGI_FORMAT m_resourceFormat;
	D3D12_CLEAR_VALUE* m_optClear;
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
	PSOManager* PSOMngr;
	IFrameResourcesManager* FrameResourceMngr;
	ResourceManager* ResourceMngr;
};

class RenderBase
{

protected:
	ID3D12Device* m_device;
	ID3D12GraphicsCommandList* m_cmdList;
	Scene* m_scene;
	PSOManager* m_psoManager;
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

	std::vector<RenderResource> m_own_resources;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_own_dsvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_own_rtvHeap;

	// "Resource interface"
	RenderResource* m_dsResource;
	std::vector<ID3D12Resource*> m_rtResources; 
	ID3D12DescriptorHeap* m_descriptorHeap; // we do not own this SRV Descriptor Heap	
	ID3D12DescriptorHeap* m_dsvHeap;
	ID3D12DescriptorHeap* m_rtvHeap;
	D3D12_GPU_DESCRIPTOR_HANDLE m_techSRVHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_textureSRVHandle;


public:
	RenderResource* create_Resource(DXGI_FORMAT resourceFormat, UINT width, UINT height, D3D12_CLEAR_VALUE* optClear=NULL);
	void create_Resource_DS(DXGI_FORMAT resourceFormat);
	void create_Resource_RT(DXGI_FORMAT resourceFormat);
	void create_DescriptorHeap_DSV();
	void create_DescriptorHeap_RTV();
	void create_DSV(DXGI_FORMAT viewFormat = DXGI_FORMAT_UNKNOWN);
	void create_RTV(DXGI_FORMAT viewFormat = DXGI_FORMAT_UNKNOWN);
	//void set_Resource_DS(ID3D12Resource* Resource);
	void set_Resource_RT(ID3D12Resource* Resource);
	void set_DescriptorHeap_RTV(ID3D12DescriptorHeap* rtvHeap); // set Descriptor Heap for RT view
	void set_DescriptorHeap(ID3D12DescriptorHeap* srvDescriptorHeap); // set Descriptor Heap for Textures SRVs

	ID3D12Resource* get_Resource(int resourceIndex);
	ID3D12Resource* get_Resource_DS();
	ID3D12Resource* get_Resource_RT();
	int get_ResourceCount();
	const ID3D12DescriptorHeap* get_rtvHeapPointer();
	const ID3D12DescriptorHeap* get_dsvHeapPointer();

	

	void initialize(const RenderMessager& renderParams); // initialize the Render
	void build(); // Do any other operations to build Render (to add more resources, create SRVs and etc)
	void draw(int flags);
	void resize(UINT newWidth, UINT newHeight);
	void releaseExternalResources();
	void connectExternalResourcesBack();
	RenderBase();
	virtual ~RenderBase();
};

