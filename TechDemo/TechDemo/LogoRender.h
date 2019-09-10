#pragma once
#include "RenderBase.h"
#include "PSOLogoLayer.h"
#include "Camera.h"
#include "Timer.h"
#include <d2d1.h>
#include <d2d1_3.h>
#include <d3d11on12.h>
#include <dwrite.h>

class LogoRender :
	public RenderBase
{
	static const int m_s_lineCount = 5;
	FrameResourcesManager<InstanceDataGPU, PassConstantsGPU, SSAO_GPU> m_frameResourceManager;
	Camera m_camera;
	Timer m_animationTimer;	
	Timer m_testTimer;	
	std::unique_ptr<Mesh> m_mesh;
	PSOLogoLayer m_psoLayer;

	IDXGISwapChain3* m_swapChain;
	ComPtr<ID3D12Resource>* m_swapChainResources;
	ID3D12CommandQueue* m_cmdQueue;
	ComPtr<ID3D12GraphicsCommandList> m_cmdListown;
	ComPtr<ID3D12CommandAllocator> m_cmdAllocator2;
	ComPtr<ID3D12Fence> m_fence;
	
	// 2D
	ID3D11On12Device* m_d3d11On12Device;
	ID3D11DeviceContext* m_d3d11Context;	
	ComPtr<ID3D11Resource>* m_wrappedBackBuffers;
	ID2D1DeviceContext* m_HUDContext;
	ComPtr<ID2D1Bitmap1>* m_HUDRenderTargets;	
	IDWriteFactory* m_writeFactory;	
	ComPtr<ID2D1SolidColorBrush> m_HUDBrush[m_s_lineCount];
	D2D_RECT_F m_textRect[m_s_lineCount];
	ComPtr<IDWriteTextFormat> m_textFormat;	

	HANDLE m_fenceEvent;
	UINT64 m_fenceValue;	
	UINT m_frameResourcesUpdatedWithCBCount;	
	float m_angle;	
	bool m_init3D_done;
	bool m_toWork;
	std::vector<std::wstring> m_guiLines;
	int m_currentLine;
		
	void build_mesh();
	void build_2DResources();
	void draw();
	void renderUI();	
	void update();
	void update_CB();
	void update_PassCB();
		
	void FlushCommandQueue();
	void setFence();
public:
	LogoRender();
	~LogoRender();

	void initialize(const RenderMessager& renderParams, const RenderMessager11on12& guiParams, ID3D12CommandQueue* cmdQueue);
	
	void build();
	void work();
	void nextLine();
	void addLine(const std::wstring& newLine);
	
	void resize(UINT newWidth, UINT newHeight);
	void setSwapChainResources(ComPtr<ID3D12Resource>* swapChainResources);

	void exit();
	
};

