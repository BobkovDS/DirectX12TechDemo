#pragma once
#include "BoneData.h"
#include <map>

class SkinnedData
{
	std::vector<BoneData*> m_root_bones; /*TO_DO: maybe one will be enough?*/ 
	std::map<std::string, BoneData*> m_bonesByNames;
	std::vector<std::string> m_animations;
	std::map<std::string, std::pair<float, float>> m_animationsBeginEndTimes;
	std::vector<DirectX::XMFLOAT4X4> m_bonesFinalTransforms;
	DirectX::XMFLOAT4X4 m_identityMatrix;
	float m_beginTimeMinForAllAnimations;
	float m_endTimeMaxForAllAnimations;
	float m_animationTime;

	void interpolate(float t, UINT animationID);
public:
	SkinnedData();
	~SkinnedData();

	void addRootBone(std::string& rootBoneName);
	BoneData* addBone(std::string& parentName, std::string newBoneName);
	BoneData* getRootBone(UINT i);
	BoneData* getBone(std::string& boneName);
	void getMinMaxBounaryTimeValues(float& bt, float& et);
	void addAnimationName(std::string animationName);
	void evaluateBeginEndTime();
	const std::vector<DirectX::XMFLOAT4X4>& getFinalTransforms(float dt, UINT animationID);
	DirectX::XMFLOAT4X4& getNodeTransform(float dt, UINT animationID);
};

