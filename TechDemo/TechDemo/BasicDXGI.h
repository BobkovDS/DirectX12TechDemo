#pragma once
#include "Canvas.h"
#include <comdef.h>
#include "stdafx.h"
#include <d2d1.h>
#include <d2d1_3.h>
#include <d3d11on12.h>
#include <dwrite.h>
#include <Initguid.h>
#include <dxgidebug.h>

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Dwrite.lib")
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "windowscodecs.lib")


using Microsoft::WRL::ComPtr;

const int g_swapChainsBufferCount = 3;

class BasicDXGI :	public Canvas
{
	// fields
	bool initDXGI_RTV_done = false;
	bool initDXGI_Full_done = false;
	
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
	DXGI_SAMPLE_DESC m_rtSampleDesc;
	ComPtr<ID3D12Fence> m_fence;
	ComPtr<IDXGIFactory4> m_factory;		
	ComPtr<ID3D12Device> m_device;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12CommandQueue> m_cmdQueue;
	ComPtr<ID3D12GraphicsCommandList> m_cmdList;
	ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
	ComPtr<ID2D1Device> m_HUDDevice;
	ComPtr<ID2D1Factory3> m_HUDFactory;	
	ComPtr<ID2D1DeviceContext> m_HUDContext;
	ComPtr<ID2D1Bitmap1> m_HUDRenderTargets[g_swapChainsBufferCount];
	ComPtr<ID2D1SolidColorBrush> m_HUDBrush;
	ComPtr<IDWriteTextFormat> m_textFormat;
	ComPtr<IDWriteTextFormat> m_textFormatBig;
	ComPtr<ID3D11On12Device> m_d3d11On12Device;	
	ComPtr<ID3D11DeviceContext> m_d3d11Context;
	ComPtr<IDWriteFactory> m_writeFactory;			
	ComPtr<ID3D12Resource> m_swapChainBuffers[g_swapChainsBufferCount];
	ComPtr<ID3D11Resource> m_wrappedBackBuffers[g_swapChainsBufferCount];

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