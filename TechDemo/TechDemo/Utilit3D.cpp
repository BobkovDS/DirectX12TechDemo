#include "Utilit3D.h"
#include "ApplException.h"

using namespace std;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

ID3D12Device* Utilit3D::m_device = nullptr;
ID3D12GraphicsCommandList* Utilit3D::m_cmdList = nullptr;
bool Utilit3D::m_initialized = false;
std::mutex Utilit3D::cdb_mutex;

Utilit3D::Utilit3D()
{	
}

Utilit3D::~Utilit3D()
{
}

void Utilit3D::initialize(ID3D12Device* iDevice, ID3D12GraphicsCommandList* iCmdList)
{
	assert(iDevice);
	assert(iCmdList);

	m_device = iDevice;
	m_cmdList = iCmdList;
	m_initialized = true;
}

ComPtr<ID3D12Resource> Utilit3D::createDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
	const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer)
{
	ComPtr<ID3D12Resource> defaultBuffer;
	HRESULT res;
	
	std::lock_guard<std::mutex> guard(cdb_mutex);

	//create the actual default buffer resource
	res = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&defaultBuffer));

	HRESULT removeReason;
	if (res != S_OK)
	{
		removeReason= device->GetDeviceRemovedReason();
		_com_error err(removeReason);
		wstring errMsg = err.ErrorMessage();

	}
	assert(SUCCEEDED(res));

	// To copy CPU memory to Default buffer, we need to create an intermediate upload heap
	res = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf()));

	assert(SUCCEEDED(res));

	//describe the data we want to copy into default buffer
	D3D12_SUBRESOURCE_DATA subResource = {};
	subResource.pData = initData;
	subResource.RowPitch = byteSize;
	subResource.SlicePitch = byteSize;

	cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST));

	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResource);

	cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ));

	return defaultBuffer;
};

ComPtr<ID3D12Resource> Utilit3D::createTextureWithData
(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const void* initData,
	UINT64 elementByteSize,
	UINT Width,
	UINT Height,
	DXGI_FORMAT textureFormat,	
	ComPtr<ID3D12Resource>& uploadBuffer,
	D3D12_RESOURCE_FLAGS textureFlags
)
{
	ComPtr<ID3D12Resource> lTextureResource;	

	UINT64 lTextureSize = Width * Height * elementByteSize;
	D3D12_RESOURCE_DESC textDesc = {};
	textDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textDesc.Alignment = 0;
	textDesc.Width = Width;
	textDesc.Height = Height;
	textDesc.DepthOrArraySize = 1;
	textDesc.MipLevels = 1;
	textDesc.Format = textureFormat;
	textDesc.SampleDesc.Count = 1;
	textDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textDesc.Flags = textureFlags;

	HRESULT res = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&lTextureResource));

	assert(SUCCEEDED(res));	

	res = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(lTextureSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf()));

	assert(SUCCEEDED(res));
	
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = Width * elementByteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch * Height;

	UpdateSubresources(m_cmdList,
		lTextureResource.Get(),
		uploadBuffer.Get(),
		0, 0, 1,
		&subResourceData);

	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		lTextureResource.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	return lTextureResource;
};

ComPtr<ID3D12Resource> Utilit3D::createDefaultBuffer(const void* initData, UINT64 byteSize, 
	ComPtr<ID3D12Resource>& uploadBuffer)
{
	assert(m_initialized); //should be initialized at first

	return Utilit3D::createDefaultBuffer(m_device, m_cmdList, initData, byteSize, uploadBuffer);
}

ComPtr<ID3DBlob> Utilit3D::compileShader(
	const wstring& filename, 
	const D3D_SHADER_MACRO* defines,
	const string& entrypoint, 
	const string& target)
{

	UINT compileflag = 0;
#if defined(DEBUG ) || defined(_DEBUG)
	compileflag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3D10Blob> errors;

	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, \
		entrypoint.c_str(), target.c_str(), compileflag, 0, &byteCode, &errors);

	if (hr != S_OK)
	{
		_com_error err(hr);
		wstring errMsg = err.ErrorMessage() + std::wstring(L" ") + filename;
		throw MyCommonRuntimeException(errMsg, L"Shader compiler");
	}

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	return byteCode;
};

ComPtr<ID3DBlob> Utilit3D::compileShader(
	const string& filename,
	const D3D_SHADER_MACRO* defines,
	const string& entrypoint,
	const string& target)
{
	wstring fileName(filename.begin(), filename.end());
	return Utilit3D::compileShader(fileName, defines, entrypoint, target);
};

void Utilit3D::UploadDDSTexture(
	std::string textureFileName,
	ComPtr<ID3D12Resource> *textureResource,
	ComPtr<ID3D12Resource> *uploader)
{
	assert(m_initialized); //should be initialized at first

	std::wstring tmpFleName(textureFileName.begin(), textureFileName.end());
	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresource;

	HRESULT hr = LoadDDSTextureFromFile(
		m_device, 
		tmpFleName.c_str(), textureResource->ReleaseAndGetAddressOf(),
		ddsData,
		subresource);

	if (hr != S_OK)
	{
		_com_error err(hr);
		wstring errMsg = err.ErrorMessage() + std::wstring(L" for ")
			+ std::wstring(textureFileName.begin(), textureFileName.end());
		throw MyCommonRuntimeException(errMsg, L"UploadDDSTexture");
	}

	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(textureResource->Get(), 0,
		static_cast<UINT>(subresource.size()));

	m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploader->GetAddressOf()));

	UpdateSubresources(m_cmdList, textureResource->Get(), uploader->Get(),
		0, 0, static_cast<UINT>(subresource.size()), subresource.data());

	m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureResource->Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, 
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE| D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

UINT Utilit3D::CalcConstantBufferByteSize(UINT byteSize)
{	
	return (byteSize + 255) & ~255;
};

// ======================================================  Non-static versions