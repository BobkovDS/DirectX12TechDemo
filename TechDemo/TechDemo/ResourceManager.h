/*
	***************************************************************************************************
	Description:
		Class to store Materials and Textures; and loads it on GPU.
			- It creates unique ID for texture's path. A Materail, if uses a texture, points to this texuture with this path ID.
			So different materials from the same/different FBX file(s), if they use textures with the same path( so actually the same
			texture) will point to one texture with the same ID. 
			- Ordering by PathID textures upload to Texture Array on GPU
			- It create SRV Descriptor Heap and SRVs for it. SRVs can are devided on two parts:
				- Technical SRVs (for Sky cub map, for SSAO maps, for Shadow map etc)
				- Applications SRVs. SRVs for textures for models.
	***************************************************************************************************
*/

#pragma once
#include "ApplDataStructures.h"
#include <map>

class ResourceManager
{
	static std::string m_dummyPath;
	std::string m_prefixName; //Prefix name for resources name (like Texture name)
	std::vector<std::unique_ptr<MaterialCPU>> m_materials;
	std::map<std::string, std::string> m_texturePathsByNames; //<texture_name, texture_path>
	std::map<std::string, int> m_texturePathID;
	std::vector<std::string> m_orderedTextureList;	
	UINT m_DummyTexturePathCount;
	bool m_textureIsUploaded;
	std::vector<ResourceWithUploader> m_textures;
	ResourceWithUploader m_materialsResource;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_textureSRVHeap;

	void buildTextureFullNameList();
public:
	ResourceManager();
	~ResourceManager();

	void setPrefixName(std::string& prefixName);
	void addMaterial(std::unique_ptr<MaterialCPU>& material);
	void addTexturePathByName(const std::string& textureName, const std::string& texturePath);
	void buildTexturePathList();
	void buildTextureSRVs();
	void loadTexture();
	void releaseTexturesUploadHeaps();
	void loadMaterials();
	int getTexturePathIDByName(const std::string& textureName);
	int addDummyTexturePath();
	ID3D12Resource* getMaterialsResource();
	ID3D12Resource* getTextureResource(UINT textureID);
	MaterialCPU* getMaterial(UINT i);
	ID3D12DescriptorHeap* getTexturesSRVDescriptorHeap();
};

