#ifndef GRAPHICSTRUCT_H
#define GRAPHICSTRUCT_H

#define MaxLights 10
#include "stdafx.h"
#include "MathHelper.h"
struct SubMesh
{
	UINT IndexCount;
	UINT VertextCount;
	UINT StartIndexLocation;
	UINT BaseVertexLocation;
	UINT MaterialID;
};

struct Mesh {
	// Information about material removed from this struct: The goal for this struct is to have only info about mesh
// Lets RenderItem has information about Mesh, Material

// Add defaultMaterialID just to connect Material with Mesh on loading step in tiny_ObjLoader
//public:
	std::string Name;
	bool IsSkinnedMesh; // Does use a mesh Skinned data?

	// These CPU buffers are used for Dynamic changes(?) and to store Mesh on CPU side for Picking, Collision 
	//and other CPU stuff with mesh. We should think about do can we use other form of storage.
	Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	UINT IndexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R32_UINT;
	UINT DefaultMaterialID = -1;
	
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	std::unordered_map<std::string, SubMesh> DrawArgs;	// Lets thinks that one mesh can have a few pieces

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView() const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = VertexByteStride;
		vbv.SizeInBytes = VertexBufferByteSize;

		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW indexBufferView() const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = IndexFormat;
		ibv.SizeInBytes = IndexBufferByteSize;

		return ibv;
	}
	void DisposeUploaders()
	{
		VertexBufferUploader = nullptr;
		IndexBufferUploader = nullptr;
	}
};

struct InstanceDataGPU {
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	UINT MaterialIndex;
private:
	UINT InstancePad0;
	UINT InstancePad1;
	UINT InstancePad2;
};


struct LightGPU
{
	DirectX::XMFLOAT3 Strength;
	float falloffStart;
	DirectX::XMFLOAT3 Direction;
	float falloffEnd;
	DirectX::XMFLOAT3 Position;
	float spotPower;
	float lightType;
	float turnOn; // to find check it for int16 or something like that
	float dummy2;
	float dummy3;
	DirectX::XMFLOAT3 ReflectDirection;
	float dummy4;
};

struct VertexGPU // Simple Vertex structure
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 UVText;
};

struct VertexExtGPU // Vertex structure with Shape ID
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 TangentU;
	DirectX::XMFLOAT2 UVText;

	UINT ShapeID; // this ID is used to find ConstatBuffer for this shape with info about matrices and material id
	//float dummy2;
	//DirectX::XMFLOAT2 dummy;
	UINT BoneIndices[16];
	float BoneWeight[16];
};

struct ResourceWithUploader
{
	Microsoft::WRL::ComPtr<ID3D12Resource> RessourceInDefaultHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> RessourceInUploadHeap = nullptr;
};

struct MaterialConstantsGPU
{
	DirectX::XMFLOAT4 DiffuseAlbedo;
	DirectX::XMFLOAT3 FresnelR0;
	float Roughness;

	// Used in texture mapping.
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
	UINT flags; //0bit - DefuseTextureIsUsed
	UINT DiffuseMapIndex[3];
};

struct ObjectContantsGPU
{
	DirectX::XMFLOAT4X4 word = MathHelper::Identity4x4();
};

struct PassConstantsGPU
{
	DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ReflectWord = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ShadowMatrix = MathHelper::Identity4x4();

	DirectX::XMFLOAT3 EyePosW = { 1.0f, 1.0f, 0.0f };
	UINT doesUseMirrorLight = 0;

	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };

	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	//DirectX::XMFLOAT4 AmbientLight = { 0.89f, 0.84f, 0.72f, 1.0f };
	//DirectX::XMFLOAT4 AmbientLight = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4 AmbientLight = { 0.3f, 0.2f, 0.2f, 1.0f };
	//DirectX::XMFLOAT4 AmbientLight = { 0,0,0,0 };

	//DirectX::XMFLOAT4 FogColor = { 0.47f, 0.47f, 0.5f, 1.0f };
	DirectX::XMFLOAT4 FogColor = { 0.0f, 0.5f, 0.4f, 1.0f };

	float FogStart = 5.0f;
	float FogRange = 50.0f;

	float CameraDistance = 0;
	float dummy2 = 0;

	LightGPU Lights[MaxLights];
};

struct SSAO_GPU
{
	DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ProjTex = MathHelper::Identity4x4();
	DirectX::XMFLOAT4 OffsetVectors[14];
	DirectX::XMFLOAT4 BlurWeight[3];
	DirectX::XMFLOAT2 InvRenderTargetSize;
	float OcclusionRadius;
	float OcclusionFadeStart;
	float OcclusionFadeEnd;
	float SurfaceEpsilon;
	DirectX::XMFLOAT2 pod;
};

#endif GRAPHICSTRUCT_H