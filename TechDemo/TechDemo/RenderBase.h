#pragma once
#include "GraphicDataStructures.h"
#include "Scene.h"
#include "PSOManager.h"
#include "FrameResourcesManager.h"

struct RenderMessager {
	ID3D12Device* Device;
	ID3D12GraphicsCommandList* CmdList;
	IDXGISwapChain* SwapChain;	
	DXGI_FORMAT DSResourceFormat; //DXGI_FORMAT_D24_UNORM_S8_UINT
	DXGI_FORMAT RTResourceFormat;
	UINT Width;
	UINT Height;
	Scene* Scene;
	PSOManager* PSOMngr;
	IFrameResourcesManager* FrameResourceMngr;
};

class RenderBase
{

protected:
	ID3D12Device* m_device;
	ID3D12GraphicsCommandList* m_cmdList;
	Scene* m_scene;
	PSOManager* m_psoManager;
	IFrameResourcesManager* m_frameResourceManager;

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

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_own_resources;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_own_dsvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_own_rtvHeap;

	// "Resource interface"
	ID3D12Resource* m_dsResource;
	std::vector<ID3D12Resource*> m_rtResources; 
	std::vector<ID3D12DescriptorHeap*> m_descriptorHeaps; // we do not own these Descriptor Heaps	
	ID3D12DescriptorHeap* m_dsvHeap;
	ID3D12DescriptorHeap* m_rtvHeap;

public:
	ID3D12Resource* create_Resource(DXGI_FORMAT resourceFormat, UINT width, UINT height);
	void create_Resource_DS(DXGI_FORMAT resourceFormat);
	void create_Resource_RT(DXGI_FORMAT resourceFormat);
	void create_DescriptorHeap_DSV();
	void create_DescriptorHeap_RTV();
	void create_DSV(DXGI_FORMAT viewFormat = DXGI_FORMAT_UNKNOWN);
	void create_RTV(DXGI_FORMAT viewFormat = DXGI_FORMAT_UNKNOWN);
	void set_Resource_DS(ID3D12Resource* Resource);
	void set_Resource_RT(ID3D12Resource* Resource);
	void set_DescriptorHeap_RTV(ID3D12DescriptorHeap* rtvHeap);

	ID3D12Resource* get_Resource(int resourceIndex);
	ID3D12Resource* get_Resource_DS();
	ID3D12Resource* get_Resource_RT();
	int get_ResourceCount();
	const ID3D12DescriptorHeap* get_rtvHeapPointer();
	const ID3D12DescriptorHeap* get_dsvHeapPointer();

	void add_DescriptorHeap();

	void initialize(const RenderMessager& renderParams); // initialize the Render
	void build(); // Do any other operations to build Render (to add more resources, create SRVs and etc)
	void draw(int flags);
	void resize(UINT newWidth, UINT newHeight);
	RenderBase();
	virtual ~RenderBase();
};

