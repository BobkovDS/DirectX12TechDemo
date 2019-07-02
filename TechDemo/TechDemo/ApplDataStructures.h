#pragma once
#include "stdafx.h"
#include <DirectXCollision.h>
#include "GraphicDataStructures.h"

#define MAX_FRAMERESOURCE_COUNT 3
#define TEXTURESCOUNT 6
enum RenderItemType {
	RIT_Opaque = 'opaq', 
	RIT_Transparent = 'tran',
	RIT_SkinnedOpaque= 'skop', 
	RIT_SkinnedNotOpaque= 'skno',
	RIT_GH = 'gesh', // object which will be processed by Geometry Shader
	RIT_Sky = 'sky' // object which will be processed by like a Sky
};

struct RenderItem {
	/*
		Represents unique scene object_mesh. It conains:
			- Object type (Opaque, Transparent, Animated and etc)
			- Mesh
			- All Instances for this unique mesh
	*/
	std::string Name;
	bool Visable;
	RenderItemType Type;
	DirectX::BoundingBox AABB;
	Mesh* Geometry = nullptr;
	std::vector<InstanceDataGPU> Instances;
};

struct MaterialCPU
{
	std::string Name;
	int MatCBIndex = -1;
	int NumFrameDirty;

	int TexturesMask = 0; // 0bit - Difuse Texture1; 1-Difuse Texture2; 2-Normal; 3-Specular; 4-TransperencyFactor; 5-none


	//Material Constant Buffer data
	DirectX::XMFLOAT4 DiffuseAlbedo = { 0.5f, 0.5f, 0.5f, 1.0f }; // this is just initialisation, diffuse color will come from obj file 
	DirectX::XMFLOAT3 FresnelR0 = { 0.5f, 0.1f, 0.1f };
	float Roughness = 0.1f;
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
	int DiffuseColorTextureIDs[TEXTURESCOUNT]; //see TextureMask for texture meaning
	bool IsTransparent; // float factor is used for Transparent factor
	bool IsTransparencyFactorUsed; // Texture is used for transparency
};

enum LightType : unsigned int {
	Directional = 0,
	Spot,
	Point,
	lightCount
};

struct CPULight
{
	// Struct for using to manipulate Light on CPU (move it, rotate, change Light parametrs). 
	// Data from this struct will copy to RenderItem - Light (to set position and directioanl) and
	// copy to ConstantBuffer Per Pass to set light parametrs for each light in lighting colculation

	std::string Name;
	bool  needToUpdateRI = false;
	bool  needToUpdateLight = false;

	float mRadius;
	float mPhi;
	float mTheta;

	float posRadius;
	float posPhi;
	float posTheta;

	DirectX::XMFLOAT3 Strength;
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Direction;
	DirectX::XMFLOAT3 ReflectDirection;
	DirectX::XMFLOAT3 initDirection;

	float falloffStart;
	float falloffEnd;
	float spotPower;
	LightType lightType;
	bool turnOn = 1;
	int shapeID = -1; //To specify which shape is used to draw this light 
	// type specifies the RenderItems, shapeId specifies shape in this RenderItem
};
