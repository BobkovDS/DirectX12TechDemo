#include "SkinnedData.h"

SkinnedData::SkinnedData()
{
}

SkinnedData::~SkinnedData()
{
}

void SkinnedData::addRootBone(std::string& rootBoneName)
{
	BoneData* lNewBone = new BoneData(rootBoneName);
	m_root_bones.push_back(lNewBone);
	m_bonesByNames[rootBoneName] = lNewBone;
}

BoneData* SkinnedData::addBone(std::string& parentName, std::string newBoneName)
{
	BoneData* lNewBone = new BoneData(newBoneName);
	m_bonesByNames[newBoneName] = lNewBone;
	m_bonesByNames[parentName]->addChild(lNewBone);
	return lNewBone;
}

BoneData* SkinnedData::getRootBone(UINT id)
{
	if (id < m_root_bones.size())
		return m_root_bones[id];
	else
		return nullptr;
}

BoneData* SkinnedData::getBone(std::string& boneName)
{
	return m_bonesByNames[boneName];
}

void SkinnedData::addAnimationName(std::string animName)
{
	m_animations.push_back(animName);
}






void SkinnedData::interpolate(float t, UINT animationID)
{
	if (animationID < m_animations.size())
	{
		for (int i = 0; i < m_root_bones.size(); i++)
			m_root_bones[i]->interpolate(t, m_animations[animationID]);
	}

	if (m_bonesFinalTransforms.size() < BoneData::getCommonIDValue()) //if we have more bones then we expect
		m_bonesFinalTransforms.resize(BoneData::getCommonIDValue());

	for (int i = 0; i < m_root_bones.size(); i++)
		m_root_bones[i]->getFinalMatrices(m_bonesFinalTransforms);
}

const std::vector<DirectX::XMFLOAT4X4>& SkinnedData::getFinalTransforms(float t, UINT animationID)
{
	interpolate(t, animationID);
	return m_bonesFinalTransforms;
}