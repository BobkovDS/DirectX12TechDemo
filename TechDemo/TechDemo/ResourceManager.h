#pragma once
#include "ApplDataStructures.h"
#include <map>

class ResourceManager
{
	std::vector<std::unique_ptr<MaterialCPU>> m_materials;
	std::map<std::string, std::string> m_texturePathsByNames;
	std::map<std::string, int> m_texturePathID;
	std::vector<std::string> m_orderedTextureList;

	bool m_textureIsUploaded;
	std::vector<ResourceWithUploader> m_textures;

	void buildTexureFullNameList();
public:
	ResourceManager();
	~ResourceManager();

	void addMaterial(std::unique_ptr<MaterialCPU>& material);
	void addTexturePathByName(const std::string& textureName, const std::string& texturePath);
	void buildTexurePathList();
	void loadTexture();
	int getTexturePathIDByName(const std::string& textureName);
};

