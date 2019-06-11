#pragma once
#include "BoneAnimation.h"
#include <map>
class BoneData
{
	DirectX::XMFLOAT4X4 m_finalTransform;
public:

	static int m_commonID;
	int m_ID;
	std::string m_name;
	DirectX::XMFLOAT4X4 m_toParentTransform; // is used for interpolation
	DirectX::XMFLOAT4X4 m_toRootTransform;
	DirectX::XMFLOAT4X4 m_bindTransform;
	std::map<std::string, BoneAnimation> m_animations;
	std::vector<BoneData*> m_childs;	

	BoneData();
	~BoneData();

	void addChild(BoneData* child);
	BoneData* getChild(UINT childID);

	void addAnimation(std::string animationName, BoneAnimation animation);
	BoneAnimation* getAnimation(std::string& animationName);

	void interpolate(float t, std::string& animationName);
	void buildToRoot(BoneData* parent);
	void getFinalMatrices(std::vector<DirectX::XMFLOAT4X4>& finalMatrices);
};

