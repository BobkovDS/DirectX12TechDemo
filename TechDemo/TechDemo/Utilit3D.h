#pragma once
#ifndef UTILIT3D_H
#define UTILIT3D_H
#include <comdef.h>
#include <assert.h>
#include "d3dx12.h"
#include "TextureLoader\DDSTextureLoader.h"
#include <D3DCompiler.h>
#include <wrl.h>

class Utilit3D
{
	ID3D12Device* m_device;
	ID3D12GraphicsCommandList* m_cmdList;
	bool m_initialized;
public:
	Utilit3D();
	~Utilit3D();
	void initialize(ID3D12Device* iDevice, ID3D12GraphicsCommandList* iCmdList);
	
	// ======================================================  Non-static versions

	//Upload mesh data to Default Heap Buffer
	/*use tamplate here to avoid of any using external data structures*/
	template<typename Mesh, typename VertexGPU, typename IndexGPU>
	void UploadMeshToDefaultBuffer(Mesh* mesh, std::vector<VertexGPU> vertexData, std::vector<IndexGPU> indexData);

	// ======================================================  Static versions

	static UINT CalcConstantBufferByteSize (UINT byteSize);

	static Microsoft::WRL::ComPtr<ID3D12Resource> createDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* initData,
		UINT64 byteSize, 
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	// for wstring file name
	static Microsoft::WRL::ComPtr<ID3DBlob> compileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);

	// for string file name
	static Microsoft::WRL::ComPtr<ID3DBlob> compileShader(
		const std::string& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);

	static void UploadDDSTexture(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		std::string textureFileName,
		Microsoft::WRL::ComPtr<ID3D12Resource> *textureResource,
		Microsoft::WRL::ComPtr<ID3D12Resource> *uploader);

	//Upload mesh data to Default Heap Buffer
	/*use tamplate here to avoid of any using external data structures*/
	template<typename Mesh, typename VertexGPU, typename IndexGPU>
	static void UploadMeshToDefaultBuffer(Mesh* mesh, std::vector<VertexGPU> vertexData, std::vector<IndexGPU> indexData,
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList );	
};

// template methods defenitions
template<typename Mesh, typename VertexGPU, typename IndexGPU>
void Utilit3D::UploadMeshToDefaultBuffer(
	Mesh* mesh, std::vector<VertexGPU> vertexData, std::vector<IndexGPU> indexData,
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList
)
{
	int vertexByteStride = sizeof(VertexGPU);

	mesh->VertexBufferByteSize = sizeof(VertexGPU) * vertexData.size();
	mesh->IndexBufferByteSize = sizeof(IndexGPU) * indexData.size();
	mesh->VertexByteStride = vertexByteStride;
	mesh->IndexFormat = DXGI_FORMAT_R32_UINT;

	mesh->VertexBufferGPU = Utilit3D::createDefaultBuffer(device, cmdList,
		vertexData.data(),
		mesh->VertexBufferByteSize,
		mesh->VertexBufferUploader);

	mesh->IndexBufferGPU = Utilit3D::createDefaultBuffer(device, cmdList,
		indexData.data(),
		mesh->IndexBufferByteSize,
		mesh->IndexBufferUploader);

	// store the same data for CPU using

	D3DCreateBlob(mesh->VertexBufferByteSize, &mesh->VertexBufferCPU);
	assert(mesh->VertexBufferCPU);
	CopyMemory(mesh->VertexBufferCPU->GetBufferPointer(), vertexData.data(), mesh->VertexBufferByteSize);

	D3DCreateBlob(mesh->IndexBufferByteSize, &mesh->IndexBufferCPU);
	assert(mesh->IndexBufferCPU);
	CopyMemory(mesh->IndexBufferCPU->GetBufferPointer(), indexData.data(), mesh->IndexBufferByteSize);
}

template<typename Mesh, typename VertexGPU, typename IndexGPU>
void Utilit3D::UploadMeshToDefaultBuffer(Mesh* mesh, std::vector<VertexGPU> vertexData, std::vector<IndexGPU> indexData)
{
	if (m_initialized)
		Utilit3D::UploadMeshToDefaultBuffer<Mesh, VertexGPU, IndexGPU>(mesh, vertexData, indexData, m_device, m_cmdList);
	else
		assert(0);
}
#endif UTILIT3D_H

