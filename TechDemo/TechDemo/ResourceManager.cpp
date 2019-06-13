#include "ResourceManager.h"
#include "Utilit3D.h"

using namespace std;

#define TECHSRVCOUNT 10

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


void ResourceManager::loadMaterials()
{
	vector<MaterialConstantsGPU> materialsGPU;
	MaterialCPU df;
	for (size_t i = 0; i < m_materials.size(); i++)
	{
		MaterialConstantsGPU tmpMat = {};
		tmpMat.DiffuseAlbedo = m_materials[i]->DiffuseAlbedo;
		tmpMat.FresnelR0 = m_materials[i]->FresnelR0;
		tmpMat.Roughness = m_materials[i]->Roughness;
		tmpMat.MatTransform = m_materials[i]->MatTransform;
		tmpMat.flags = m_materials[i]->TexturesMask;
		
		for (int ti = 0; ti < TEXTURESCOUNT; ti++)
				tmpMat.DiffuseMapIndex[ti] = m_materials[i]->DiffuseColorTextureIDs[ti];

		materialsGPU.push_back(tmpMat);
	}

	m_materialsResource.RessourceInDefaultHeap = Utilit3D::createDefaultBuffer(	
		materialsGPU.data(),
		materialsGPU.size() * sizeof(MaterialConstantsGPU),
		m_materialsResource.RessourceInUploadHeap);
}
ID3D12Resource* ResourceManager::getMaterialsResource()
{
	return m_materialsResource.RessourceInDefaultHeap.Get();
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

void ResourceManager::buildTexturePathList()
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

void ResourceManager::buildTextureFullNameList()
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
	buildTextureFullNameList();

	assert(m_textureIsUploaded == false); //this function should be executed once
	
	m_textures.resize(m_orderedTextureList.size());

	for (int i = 0; i < m_orderedTextureList.size(); i++)	
		Utilit3D::UploadDDSTexture(m_orderedTextureList[i], 
			&m_textures[i].RessourceInDefaultHeap, &m_textures[i].RessourceInUploadHeap);
	
	m_textureIsUploaded = true;

	buildTextureSRVs();
}



void ResourceManager::buildTextureSRVs()
{
	// 1) Create SRV Descriptor Heap
	{				
		int lTextureCount = m_textures.size();
		if (lTextureCount == 0) lTextureCount = 1;
		int lDescriptorCount = TECHSRVCOUNT + lTextureCount;
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {}; // Used for Models Texture
		srvHeapDesc.NumDescriptors = lDescriptorCount;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		srvHeapDesc.NodeMask = 0;
		
		Utilit3D::getDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_textureSRVHeap));		
	}

	// 2) Prepare...
	UINT lSrvSize = Utilit3D::getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE lhDescriptor(m_textureSRVHeap->GetCPUDescriptorHandleForHeapStart());

	lhDescriptor.Offset(TECHSRVCOUNT, lSrvSize); // skip [TECHSRVCOUNT] SRVs for technical SRVs. It will be create in different Renders

	// 3) Create SRV for texture resources
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;	
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;	
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	if (m_textures.size()==0) 
	{ 
		/*
			If we do not have no one Texture, let's create one dummy SRV fot no resource
		*/
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.Texture2D.MipLevels = 1;
		Utilit3D::getDevice()->CreateShaderResourceView(nullptr,
			&srvDesc, lhDescriptor);
	}
	else
	{
		for (int i = 0; i < m_textures.size(); i++)
		{
			srvDesc.Format = m_textures[i].RessourceInDefaultHeap->GetDesc().Format;
			srvDesc.Texture2D.MipLevels = m_textures[i].RessourceInDefaultHeap->GetDesc().MipLevels;

			Utilit3D::getDevice()->CreateShaderResourceView(m_textures[i].RessourceInDefaultHeap.Get(),
				&srvDesc, lhDescriptor);

			lhDescriptor.Offset(1, lSrvSize);
		}
	}
}

ID3D12DescriptorHeap* ResourceManager::getTexturesSRVDescriptorHeap()
{	
	return m_textureSRVHeap.Get();
}