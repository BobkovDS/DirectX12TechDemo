#pragma once
#include "BoneData.h"

class SkinnedData
{
	std::vector<BoneData*> m_root_bones;
	std::vector<std::string> m_animations;
	std::vector<DirectX::XMFLOAT4X4> m_bonesFinalTransforms;	

	void interpolate(float t, UINT animationID);
public:
	SkinnedData();
	~SkinnedData();

	void addRootBone(BoneData* root);
	BoneData* getRootBone(UINT i);

	const std::vector<DirectX::XMFLOAT4X4>& getFinalTransforms(float t, UINT animationID);
};

