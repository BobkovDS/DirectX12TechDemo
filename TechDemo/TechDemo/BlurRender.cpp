#include "BlurRender.h"

using namespace DirectX;

BlurRender::BlurRender() : m_isBlurReady(false)
{
}

BlurRender::~BlurRender()
{
}

void BlurRender::initialize(const RenderMessager& renderParams)
{
	RenderBase::initialize(renderParams);
}

void BlurRender::setInputResource(ID3D12Resource* inputResource)
{
	m_inputResource = inputResource;
}

void BlurRender::build(int blurCount)
{
	assert(m_initialized == true);

	m_blurCount = blurCount;
	if (m_blurCount % 2 != 0)
		m_blurCount++; // make it even, that the last draw call was back to inputResource
	
	// Two UAV Resources
	{
		create_ResrourcesUAV(m_rtResourceFormat); // for result/work Blur				
	}

	// Initialize PSO layer
	DXGI_SAMPLE_DESC lSampleDesc;
	lSampleDesc.Count = 1;
	lSampleDesc.Quality= 0;

	m_psoLayer.buildPSO(m_device, m_rtResourceFormat, m_dsResourceFormat, lSampleDesc);
		
	build_TechDescriptors();	
	m_timer.setTickTime(0.0083f); // 60 fps
	m_isBlurReady = false;
}

void BlurRender::create_ResrourcesUAV(DXGI_FORMAT resourceFormat)
{
	m_own_resources.resize(2);
	
	// "A" Resource
	m_uavResources.push_back(create_Resource(resourceFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));
	// "B" Resource
	m_uavResources.push_back(create_Resource(resourceFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));
}

void BlurRender::build_TechDescriptors()
{
	m_isBlurReady = false;
	UINT lSrvSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE lhDescriptor(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart());
	lhDescriptor.Offset(TECHSLOT_BLUR_A_SRV, lSrvSize);

	// Create SRV/UAVs for both resources
	{			
		/*
		MAPPING:
			A - SRV \_ Horizontal Call
			B - UAV /
			B - SRV \_ Vertical Call
			A - UAV /
		*/
		
		// A - SRV		
		m_device->CreateShaderResourceView(m_uavResources[0]->getResource(), nullptr, lhDescriptor);
		// B - UAV
		lhDescriptor.Offset(1, lSrvSize);
		m_device->CreateUnorderedAccessView(m_uavResources[1]->getResource(), nullptr, nullptr, lhDescriptor);
			
		// B - SRV
		lhDescriptor.Offset(1, lSrvSize);
		m_device->CreateShaderResourceView(m_uavResources[1]->getResource(), nullptr, lhDescriptor);
		// A - UAV
		lhDescriptor.Offset(1, lSrvSize);		
		m_device->CreateUnorderedAccessView(m_uavResources[0]->getResource(), nullptr, nullptr, lhDescriptor);		
	}

	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE lhGpuDescriptor(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart());
		CD3DX12_GPU_DESCRIPTOR_HANDLE lhGpuDescriptor2 = lhGpuDescriptor;

		lhGpuDescriptor.Offset(TECHSLOT_SSAO_VSN, lSrvSize);
		m_techSRVHandle = lhGpuDescriptor; // Points to ViewNormal and Depths Maps

		lhGpuDescriptor = lhGpuDescriptor2;
		lhGpuDescriptor.Offset(TECHSLOT_BLUR_A_SRV, lSrvSize);
		m_gpu_HorizontalCall_Handle = lhGpuDescriptor;
		lhGpuDescriptor.Offset(2, lSrvSize);
		m_gpu_VerticalCall_Handle= lhGpuDescriptor;
	}

	m_uavResources[0]->changeState(m_cmdList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_uavResources[1]->changeState(m_cmdList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void BlurRender::resize(UINT iwidth, UINT iheight)
{
	m_isBlurReady = false;

	RenderBase::resize(iwidth/2, iheight/2);
	m_uavResources[0]->resize(m_width, m_height);
	m_uavResources[1]->resize(m_width, m_height);

	build_TechDescriptors();
}

void BlurRender::draw(int flags)
{
	//if (!m_timer.tick()) return; // TO_DO: delete

	// Copy SSAO AO Map Input Resource to "our" A resource
	{
		{
			const D3D12_RESOURCE_BARRIER lBarries[] = {

				CD3DX12_RESOURCE_BARRIER::Transition(m_inputResource,
				D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE),
				m_uavResources[0]->getBarrier(D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_STATE_COPY_DEST)
			};

			m_cmdList->ResourceBarrier(2, lBarries);
		}	
		
		m_cmdList->CopyResource(m_uavResources[0]->getResource(), m_inputResource);

		{
			const D3D12_RESOURCE_BARRIER lBarries[] = {

				CD3DX12_RESOURCE_BARRIER::Transition(m_inputResource,
				D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ),
				m_uavResources[0]->getBarrier(D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_GENERIC_READ),
				m_uavResources[1]->getBarrier(D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
			};

			m_cmdList->ResourceBarrier(3, lBarries);
		}

	}

	m_cmdList->SetComputeRootSignature(m_psoLayer.getRootSignature());		
	m_cmdList->SetPipelineState(m_psoLayer.getPSO(OPAQUELAYER));

	// Set DescriptorHeaps  
	ID3D12DescriptorHeap* ldescriptorHeaps[] = { m_descriptorHeap };
	m_cmdList->SetDescriptorHeaps(1, ldescriptorHeaps);
	
	// Set RootArguments	
	auto passCB = m_frameResourceManager->getCurrentSSAOCBResource();	
	m_cmdList->SetComputeRootDescriptorTable(2, m_techSRVHandle);
	m_cmdList->SetComputeRootConstantBufferView(3, passCB->GetGPUVirtualAddress()); // Pass constant buffer data
	
	for (int i = 0; i < m_blurCount; i++)
	{		
		// Horizontal blur pass. A-input(SRV), B-output(UAV). 		
		{
			m_cmdList->SetComputeRoot32BitConstant(0, 1, 0); // Tech Flags: Horiztontal Pass (TRUE)
			m_cmdList->SetComputeRootDescriptorTable(1, m_gpu_HorizontalCall_Handle);

			UINT numGroups = (UINT)ceilf(m_width / 256.0f);
			m_cmdList->Dispatch(numGroups, m_height, 1);

			{
				const D3D12_RESOURCE_BARRIER lBarries[] = {
					m_uavResources[0]->getBarrier(D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
					m_uavResources[1]->getBarrier(D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_GENERIC_READ),
				};

				m_cmdList->ResourceBarrier(2, lBarries);
			}	
		}

		// Vertical blur pass. A-output(UAV), B-input(SRV). 
		{
			m_cmdList->SetComputeRoot32BitConstant(0, 0, 0); // Tech Flags: Horiztontal Pass (FALSE)
			m_cmdList->SetComputeRootDescriptorTable(1, m_gpu_VerticalCall_Handle);

			UINT numGroups = (UINT)ceilf(m_height/ 256.0f);
			m_cmdList->Dispatch(numGroups, m_width, 1);

			{
				const D3D12_RESOURCE_BARRIER lBarries[] = {
						m_uavResources[0]->getBarrier(D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_GENERIC_READ),
						m_uavResources[1]->getBarrier(D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
				};

				m_cmdList->ResourceBarrier(2, lBarries);
			}
		}
	}
	
	m_uavResources[1]->changeState(m_cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);

	m_isBlurReady = true;
}

void BlurRender::build_screen()
{
	// TO_DO: do we need it? Maybe delete ?

	std::vector<VertexGPU> lVertices(4);
	std::vector<uint32_t> lIndices(6);

	VertexGPU vertex = {};

	//v0
	vertex.Pos = XMFLOAT3(-1.0f, -1.0f, 0.0f);
	vertex.Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex.TangentU = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
	vertex.UVText = XMFLOAT2(0.0f, 1.0f);
	lVertices[0] = vertex;
	//v1
	vertex.Pos = XMFLOAT3(-1.0f, 1.0f, 0.0f);
	vertex.Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex.UVText = XMFLOAT2(0.0f, 0.0f);
	lVertices[1] = vertex;
	//v2
	vertex.Pos = XMFLOAT3(1.0f, 1.0f, 0.0f);
	vertex.Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex.UVText = XMFLOAT2(1.0f, 0.0f);
	lVertices[2] = vertex;
	//v3
	vertex.Pos = XMFLOAT3(1.0f, -1.0f, 0.0f);
	vertex.Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex.UVText = XMFLOAT2(1.0f, 1.0f);
	lVertices[3] = vertex;

	lIndices[0] = 0;
	lIndices[1] = 1;
	lIndices[2] = 2;

	lIndices[3] = 0;
	lIndices[4] = 2;
	lIndices[5] = 3;

	m_mesh = std::make_unique<Mesh>();
	int vertexByteStride = sizeof(VertexGPU);

	
	m_mesh->VertexBufferByteSize = vertexByteStride * (UINT) lVertices.size();
	m_mesh->IndexBufferByteSize = sizeof(uint32_t) * (UINT) lIndices.size();
	m_mesh->VertexByteStride = vertexByteStride;
	m_mesh->IndexFormat = DXGI_FORMAT_R32_UINT;

	SubMesh submesh = {};
	m_mesh->Name = "screen";
	submesh.IndexCount = (UINT) lIndices.size();
	m_mesh->DrawArgs[m_mesh->Name] = submesh;

	Utilit3D::UploadMeshToDefaultBuffer<Mesh, VertexGPU, uint32_t>(m_mesh.get(), lVertices, lIndices);
}
