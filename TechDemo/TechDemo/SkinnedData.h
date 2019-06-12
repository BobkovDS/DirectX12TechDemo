#pragma once
#include "BoneData.h"
#include <map>

class SkinnedData
{
	std::vector<BoneData*> m_root_bones; /*TO_DO: maybe one will be enough?*/ 
	std::map<std::string, BoneData*> m_bonesByNames;
	std::vector<std::string> m_animations;
	std::vector<DirectX::XMFLOAT4X4> m_bonesFinalTransforms;	

	void interpolate(float t, UINT animationID);
public:
	SkinnedData();
	~SkinnedData();

	void addRootBone(std::string& rootBoneName);
	BoneData* addBone(std::string& parentName, std::string newBoneName);
	BoneData* getRootBone(UINT i);
	BoneData* getBone(std::string& boneName);
	void addAnimationName(std::string animationName);

	const std::vector<DirectX::XMFLOAT4X4>& getFinalTransforms(float t, UINT animationID);
};

