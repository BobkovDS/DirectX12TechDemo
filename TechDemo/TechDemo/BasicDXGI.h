#pragma once
#include "Canvas.h"
#include <comdef.h>
#include "stdafx.h"

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

class BasicDXGI :	public Canvas
{
	// fields
	bool initDXGI_RTV_done = false;
	bool initDXGI_Full_done = false;
	static const int m_swapChainsBufferCount=2;	
	DXGI_FORMAT m_backBufferFormat;
	UINT m_rtvDescriptorSize = 0;
	HANDLE m_fenceEvent;
	UINT64 m_fenceValue;	
	float m_dxKoef = 0.8f;
	float m_dyKoef = 0.8f;

	// methods
	void getHardwareAdapter(IDXGIFactory2* pFactory, ComPtr<IDXGIAdapter1>& ppAdapter);
	void create_RTV();
	void logAdapters();
	void logAdapterOutput(IDXGIAdapter1* pAdapter);
	void logOutputDisplayMode(IDXGIOutput* pOutput, DXGI_FORMAT format);

protected:
	bool m_fullScreen= false;
	ComPtr<ID3D12Fence> m_fence;
	ComPtr<IDXGIFactory4> m_factory;	
	ComPtr<ID3D12Device> m_device;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12CommandQueue> m_cmdQueue;
	ComPtr<ID3D12GraphicsCommandList> m_cmdList;
	ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
	
	ComPtr<ID3D12Resource> m_swapChainBuffers[m_swapChainsBufferCount];

	ComPtr<ID3D12DescriptorHeap> m_rtvHeap; // Render Target View DescriptorHeap

public:
	BasicDXGI(HINSTANCE hInstance, const std::wstring& applName = L"Basic DXGI application", int width=500, int height=500);
	~BasicDXGI();
	
	void init3D(); // calls after window init and before window run
	void run();
	void work();
	virtual void beforeDraw(){};
	void draw();
	virtual void afterDraw(){};
	void onKeyDown(WPARAM btnState);
	void onReSize(int newWidth, int newHeight);
	void FlushCommandQueue();
	void setFence();

	UINT64 getFenceValue() {return m_fenceValue;}
	UINT rtvDescriptorSize() { return m_rtvDescriptorSize; }
	DXGI_FORMAT backBufferFormat() { return m_backBufferFormat; }
	HRESULT m_prevPresentCallStatus = S_OK;
	
};

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif

#define _LOGDEBUG