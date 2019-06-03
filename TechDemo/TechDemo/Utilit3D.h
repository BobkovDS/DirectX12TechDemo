#pragma once
#ifndef UTILIT3D_H
#define UTILIT3D_H
#include <comdef.h>
#include <assert.h>
#include "d3dx12.h"
#include "TextureLoader\DDSTextureLoader.h"
#include <D3DCompiler.h>
#include "DataStructures.h"

class Utilit3D
{
public:
	Utilit3D();
	~Utilit3D();

	static UINT Utilit3D::CalcConstantBufferByteSize(UINT byteSize);

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
};
#endif UTILIT3D_H