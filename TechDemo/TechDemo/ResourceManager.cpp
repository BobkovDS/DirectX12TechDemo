#include "ResourceManager.h"
#include "Utilit3D.h"

using namespace std;

ResourceManager::ResourceManager()
{
}


ResourceManager::~ResourceManager()
{
}

void ResourceManager::addMaterial(std::unique_ptr<MaterialCPU>& material)
{
	m_materials.push_back(std::move(material));
}

void ResourceManager::addTexturePathByName(const string& textureName, const string& texturePath)
{
	auto it = m_texturePathsByNames.find(textureName);
	if (it == m_texturePathsByNames.end())
		m_texturePathsByNames[textureName] = texturePath;
}

int ResourceManager::getTexturePathIDByName(const string& textureName)
{
	auto it1 = m_texturePathsByNames.find(textureName);
	assert(it1 != m_texturePathsByNames.end());

	string& lTexutrePath = m_texturePathsByNames[textureName];
	auto it2 = m_texturePathID.find(lTexutrePath);
	assert(it2 != m_texturePathID.end());

	return m_texturePathID[lTexutrePath];
}

void ResourceManager::buildTexurePathList()
{
	/*
		Create a list of unique texture names. So later we will upload on GPU each texture only once.
		Materials will use this list to map their Texture_name with Texture_ID, where Texture_ID is position of this 
		texture in Texture array on GPU.
	*/

	// Build unique texture names list and set ID for it
	{
		int ltextureID = 0;
		auto it_begin = m_texturePathsByNames.begin();
		for (; it_begin != m_texturePathsByNames.end(); it_begin++)
		{
			if (m_texturePathID.find(it_begin->second) == m_texturePathID.end())
				m_texturePathID[it_begin->second] = ltextureID++;
		}
	}
}


void ResourceManager::buildTexureFullNameList()
{
	// copy Texture name map to vector and send this vector to up	
	m_orderedTextureList.resize(m_texturePathID.size());
	auto texture_it = m_texturePathID.begin();
	for (; texture_it != m_texturePathID.end(); texture_it++)
	{
		string texture_path = (*texture_it).first;
		texture_path.replace(texture_path.size() - 3, 3, "DDS");
		m_orderedTextureList[texture_it->second] = texture_path;
	}	
}


void ResourceManager::loadTexture()
{
	void buildTexureFullNameList();

	assert(m_textureIsUploaded == false); //this function should be executed once
	
	m_textures.resize(m_orderedTextureList.size());

	for (int i = 0; i < m_orderedTextureList.size(); i++)	
		Utilit3D::UploadDDSTexture(m_orderedTextureList[i], 
			&m_textures[i].RessourceInDefaultHeap, &m_textures[i].RessourceInUploadHeap);
	
	m_textureIsUploaded = true;
}