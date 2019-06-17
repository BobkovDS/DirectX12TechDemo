#pragma once
#include "BoneAnimation.h"
#include <map>
class BoneData
{
	DirectX::XMFLOAT4X4 m_finalTransform;
	static int m_commonID;
	int m_ID;
	std::string m_name;
	std::map<std::string, BoneAnimation> m_animations;
	std::vector<BoneData*> m_childs;
public:
	DirectX::XMFLOAT4X4 m_toParentTransform; 
	DirectX::XMFLOAT4X4 m_toParentTransformAnimated; // is used for interpolation
	DirectX::XMFLOAT4X4 m_toRootTransform;
	DirectX::XMFLOAT4X4 m_bindTransform;
	

	BoneData(std::string name);
	~BoneData();

	void addChild(BoneData* child);
	BoneData* getChild(UINT childID);

	void addAnimation(std::string animationName, BoneAnimation animation);
	BoneAnimation* getAnimation(std::string& animationName);

	void interpolate(float t, std::string& animationName);
	void buildToRoot(BoneData* parent);
	void getFinalMatrices(std::vector<DirectX::XMFLOAT4X4>& finalMatrices);
	void get_begin_end_animationTime(std::string& animationName, float& beginT, float& endT);
	static int getCommonIDValue() { return m_commonID; }
};

