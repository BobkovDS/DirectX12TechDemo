#include "LogoRender.h"
using namespace DirectX;

LogoRender::LogoRender()
{	
	m_frameResourcesUpdatedWithCBCount = MAX_FRAMERESOURCE_COUNT;
}

LogoRender::~LogoRender()
{
	m_toWork = false;	
	FlushCommandQueue();
	CloseHandle(m_fenceEvent);
	//m_HUDContext = nullptr;
}

void LogoRender::initialize(const RenderMessager& renderParams, const RenderMessager11on12& guiParams, ID3D12CommandQueue* cmdQueue)
{
	m_cmdQueue = cmdQueue;	
	RenderBase::initialize(renderParams);

	m_d3d11On12Device = guiParams.D3d11On12Device;
	m_d3d11Context = guiParams.D3d11Context;
	m_HUDContext = guiParams.HUDContext;
	m_writeFactory = guiParams.WriteFactory;
	m_wrappedBackBuffers = guiParams.WrappedBackBuffers;
	m_HUDRenderTargets = guiParams.HUDRenderTargets;
}

void LogoRender::build()
{
	assert(m_initialized == true);

	HRESULT res;

	//build_mesh();

	// Create Fence and Event Handle for it
	{
		m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
		m_fenceValue = 0;

		m_fenceEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		assert(m_fenceEvent != nullptr);
	}

	//Create Command Objects;
	{
		//Command list allocator
		res = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator2));
		assert(SUCCEEDED(res));

		// Create Command List (here we think that one Command list is enough for us)
		res = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAllocator2.Get(),
			nullptr, IID_PPV_ARGS(&m_cmdListown));
		assert(SUCCEEDED(res));	
		//m_cmdList = m_cmdListown.Get();		
	}
	
	// DepthStencil resources. This class own it.
	m_own_resources.resize(1);	
	{
		create_DescriptorHeap_DSV();
		create_Resource_DS(m_dsResourceFormat);
		create_DSV();
	}

	// Frame resources
	m_frameResourceManager.Initialize(m_device, m_fence.Get(), 1, 1, 0, 0, 0);

	// Initialize PSO layer
	DXGI_SAMPLE_DESC lSampleDesc;
	lSampleDesc.Count = 1;// m_msaaRenderTargets->getSampleCount();
	lSampleDesc.Quality = 0;// m_msaaRenderTargets->getSampleQuality();
	m_psoLayer.buildPSO(m_device, m_rtResourceFormat, m_dsResourceFormat, lSampleDesc);
	
	build_mesh();		

	// create Camera	
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(4.0, 0.0f, 0.0f, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorSet(0.0, 0.0f, 0.0f, 1.0f);
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_camera.lookAt(pos, target, up);

	float w = static_cast<float>(900);
	float h = static_cast<float>(900);
	float aspect = w / h;
	m_camera.lens->setLens(0.25f*XM_PI, aspect, 1.0f, 100.0f);	
	m_camera.buildFrustumBounding();
	
	m_animationTimer.setTickTime(0.016f);
	m_testTimer.setTickTime(1.0f);
	m_cmdListown->Close();

	// Create D2D/DWrite objects for rendering text	
#ifdef GUI_HUD
	build_2DResources();
#endif

	m_toWork = true;	
}

void LogoRender::work()
{
	while (m_toWork)
	{		
		if (m_animationTimer.tick())
		{
			// here we begin new Graphic Frame
			m_frameResourceManager.getFreeFR(); // so we need new Frame resource

			// for graphic frame we use CommandAllocator from FR
			m_frameResourceManager.changeCmdAllocator(m_cmdListown.Get(), nullptr);

			// update data
			update();

			// draw data
			draw();

			m_cmdListown->Close();
			ID3D12CommandList* CmdLists[] = { m_cmdListown.Get() };
			m_cmdQueue->ExecuteCommandLists(1, CmdLists);

#ifdef GUI_HUD
			renderUI();
#endif
			
			m_swapChain->Present(0, 0);
			//res = m_cmdListown->Reset(m_cmdAllocator2.Get(), NULL);
			

			m_frameResourceManager.currentFR()->setFenceValue(m_fenceValue);
			setFence();			
		}
	}
}

void LogoRender::draw()
{
	// Set RootArguments
	auto objectCB = m_frameResourceManager.getCurrentObjectCBResource();
	auto passCB = m_frameResourceManager.getCurrentPassCBResource();	

	int lResourceIndex = m_swapChain->GetCurrentBackBufferIndex();
	m_cmdListown->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainResources[lResourceIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE currentRTV(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		lResourceIndex, m_rtvDescriptorSize);

	m_cmdListown->RSSetViewports(1, &m_viewPort);
	m_cmdListown->RSSetScissorRects(1, &m_scissorRect);		

	m_cmdListown->OMSetRenderTargets(1, &currentRTV, true, nullptr);
	FLOAT clearColor[4] = { 0, 0, 0, 1.0f };
	m_cmdListown->ClearRenderTargetView(currentRTV, clearColor, 0, nullptr);
	m_cmdListown->SetPipelineState(m_psoLayer.getPSO(OPAQUELAYER));	
	
	m_cmdListown->SetGraphicsRootSignature(m_psoLayer.getRootSignature());
	m_cmdListown->SetGraphicsRootShaderResourceView(0, objectCB->GetGPUVirtualAddress()); // Instances constant buffer arrray data
	m_cmdListown->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress()); // Pass constant buffer data

	m_cmdListown->IASetVertexBuffers(0, 1, &m_mesh->vertexBufferView());
	m_cmdListown->IASetIndexBuffer(&m_mesh->indexBufferView());
	m_cmdListown->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);

	//Draw  call
	auto drawArg = m_mesh->DrawArgs[m_mesh->Name];
	m_cmdListown->DrawIndexedInstanced(drawArg.IndexCount, 1, drawArg.StartIndexLocation, 0, 0);
	
#ifndef GUI_HUD
	// If we draw GUI, no need to change statut from RENDER_TARGET to PRESENT, gui render function will do it
	m_cmdListown->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_swapChainResources[lResourceIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));
#endif
	
}

void LogoRender::renderUI()
{
	int lResourceIndex = m_swapChain->GetCurrentBackBufferIndex();		

	m_d3d11On12Device->AcquireWrappedResources(m_wrappedBackBuffers[lResourceIndex].GetAddressOf(), 1);

	m_HUDContext->SetTarget(m_HUDRenderTargets[lResourceIndex].Get());
	m_HUDContext->BeginDraw();
	m_HUDContext->SetTransform(D2D1::Matrix3x2F::Identity());
	
	for (int i = 0; i < m_s_lineCount; i++)
	{		
		int lLineID = m_currentLine - i + 1;
		if (lLineID >= 0 && lLineID < m_guiLines.size())
		{
			m_HUDContext->DrawTextW(
				m_guiLines[lLineID].c_str(),
				m_guiLines[lLineID].length(),
				m_textFormat.Get(),
				m_textRect[i],
				m_HUDBrush[i].Get());
		}		
	}

	HRESULT res = m_HUDContext->EndDraw();
	m_d3d11On12Device->ReleaseWrappedResources(m_wrappedBackBuffers[lResourceIndex].GetAddressOf(), 1);
	m_d3d11Context->Flush();
}

void LogoRender::nextLine()
{
	m_currentLine++;
	if (m_currentLine >= m_guiLines.size())
		m_currentLine = m_guiLines.size();
}

void LogoRender::addLine(const std::wstring& newLine)
{
	m_guiLines.push_back(newLine);
	m_currentLine = m_guiLines.size()-1;
}

void LogoRender::update()
{
	update_CB();
	update_PassCB();
}

void LogoRender::update_CB()
{
	//if (m_frameResourcesUpdatedWithCBCount == 0) return;
	m_angle += 0.005f;

	auto currCBObject = m_frameResourceManager.currentFR()->getObjectCB();
	
	InstanceDataGPU lInstData = {};	
	DirectX::XMMATRIX lInstWorldM = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX lRotationM = DirectX::XMMatrixRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), m_angle);
	lInstWorldM = lInstWorldM * lRotationM;

	DirectX::XMStoreFloat4x4(&lInstData.World, lInstWorldM);	
	currCBObject->CopyData(0, lInstData);
	
	m_frameResourcesUpdatedWithCBCount--;
}

void LogoRender::update_PassCB()
{
	auto mMainPassCB = m_frameResourceManager.tmpPassConsts;	

	XMMATRIX view = m_camera.getView();
	XMMATRIX proj = m_camera.lens->getProj();	
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	
	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));	
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));	

	mMainPassCB.EyePosW = m_camera.getPosition3f();
	mMainPassCB.TotalTime = m_animationTimer.totalTime();

	const std::vector<LightCPU> lights;

	for (size_t i = 0; i < lights.size(); i++)
	{
		mMainPassCB.Lights[i].Direction = lights.at(i).Direction;
		mMainPassCB.Lights[i].Strength = lights.at(i).Color;
		mMainPassCB.Lights[i].Position = lights.at(i).Position;
		mMainPassCB.Lights[i].spotPower = lights.at(i).spotPower;
		mMainPassCB.Lights[i].falloffStart = lights.at(i).falloffStart;
		mMainPassCB.Lights[i].falloffEnd = lights.at(i).falloffEnd;
		mMainPassCB.Lights[i].lightType = lights.at(i).lightType + 1; // 0 - is undefined type of light
		mMainPassCB.Lights[i].turnOn = lights.at(i).turnOn;		
	}

	auto currPassCB = m_frameResourceManager.currentFR()->getPassCB();
	currPassCB->CopyData(0, mMainPassCB);
}

void LogoRender::build_mesh()
{
	std::vector<VertexGPU> lVertices;
	std::vector<uint32_t> lIndices;

	VertexGPU lVertex = {};	
	int lcpc = 15; // Circle Points count
	int lCc = 20; // Circles count
	float lRadius = 0.5f;
	float lRadDz = 6.28f / (float)(lcpc-1); // angle dx in Radians
	float lRadDy = 6.28f / (float)(lCc -1); // angle dx in Radians

	for (int k = 0; k < lCc; k++)
	{
		for (int i = 0; i < lcpc; i++)
		{
			float lTheta = lRadDz * i;
			float lAlpha = lRadDy * k;

			float x = lRadius * sin(lTheta) * cos(lAlpha);
			float y = lRadius * cos(lTheta);
			float z = lRadius * sin(lTheta) * sin(lAlpha); 

			lVertex.Pos = XMFLOAT3(x, y, z);
			//lVertex.TangentU = XMFLOAT3(0.5f - cos(lTheta)*0.5, 0.5f - sin(lTheta)*0.5, 0.0f);
			lVertex.TangentU = XMFLOAT4(0.5f + x*0.5, 0, 0.5f + z * 0.5, 0.0f);
			lVertices.push_back(lVertex);
			lIndices.push_back(k*lcpc + i);
		}
	}

	m_mesh = std::make_unique<Mesh>();
	int vertexByteStride = sizeof(VertexGPU);

	m_mesh->VertexBufferByteSize = vertexByteStride * lVertices.size();
	m_mesh->IndexBufferByteSize = sizeof(uint32_t) * lIndices.size();
	m_mesh->VertexByteStride = vertexByteStride;
	m_mesh->IndexFormat = DXGI_FORMAT_R32_UINT;

	SubMesh submesh = {};
	m_mesh->Name = "logo";
	submesh.IndexCount = lIndices.size();
	m_mesh->DrawArgs[m_mesh->Name] = submesh;

	Utilit3D::UploadMeshToDefaultBuffer<Mesh, VertexGPU, uint32_t>(m_mesh.get(), lVertices, lIndices);
}

void LogoRender::build_2DResources()
{
	//m_guiLines.push_back(L"Loading of FBX file: Scene");
	//m_guiLines.push_back(L"Loading of FBX file: Light");
	//m_guiLines.push_back(L"Loading of FBX file: Camera");
	//m_guiLines.push_back(L"Textures loading");
	//m_guiLines.push_back(L"Materials loading");
	//m_guiLines.push_back(L"Scene building");
	//m_guiLines.push_back(L"Renders building");


	m_writeFactory->CreateTextFormat(
		L"Verdana",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		20,
		L"en-us",
		&m_textFormat);
	m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

	D2D_SIZE_F lrtSize = m_HUDRenderTargets[0]->GetSize();

	D2D1::ColorF lColors[m_s_lineCount] =
	{
		D2D1::ColorF(0.0f, 0.5f, 0.4f, 1.0f),
		D2D1::ColorF(1.0f, 0.5f, 0.3f, 1.0f),
		D2D1::ColorF(0.0f, 0.5f, 0.2f, 1.0f),
		D2D1::ColorF(0.0f, 0.5f, 0.1f, 1.0f),
		D2D1::ColorF(0.0f, 0.5f, 0.0f, 1.0f)
	};

	for (int i = 0; i < m_s_lineCount; i++)
	{
		if (i==1)
			m_textRect[i] = D2D1::RectF(90, 200 - i*25, lrtSize.width, lrtSize.height);
		else
			m_textRect[i] = D2D1::RectF(100 + i*10, 200 - i * 25, lrtSize.width, lrtSize.height);

		m_HUDContext->CreateSolidColorBrush(lColors[i], &m_HUDBrush[i]);
	}
}

void LogoRender::resize(UINT iwidth, UINT iheight)
{
	RenderBase::resize(iwidth, iheight);
}

void LogoRender::setSwapChainResources(ComPtr<ID3D12Resource>* swapChainResources)
{
	m_swapChainResources = swapChainResources;
}

void LogoRender::FlushCommandQueue()
{
	/*
		Lets wait for when all command to this point are done in command queue.
		It is reuqired for that all resources and commands in commandAllocator
		are used and can be cleaned (reseted)
	*/
	
	m_fenceValue++;

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	m_cmdQueue->Signal(m_fence.Get(), m_fenceValue);

	//Wait until the GPU has completed up to this new fence point
	if (m_fence->GetCompletedValue() < m_fenceValue)
	{
		m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

void LogoRender::setFence()
{
	if (m_fence == NULL) return;

	m_cmdQueue->Signal(m_fence.Get(), m_fenceValue);
	m_fenceValue++;
}
void LogoRender::exit()
{
	m_toWork = false;
}
