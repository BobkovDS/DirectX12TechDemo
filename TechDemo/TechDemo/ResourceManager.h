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

