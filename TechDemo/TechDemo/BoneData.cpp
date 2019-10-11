#include "BoneData.h"

using namespace DirectX;

int BoneData::m_commonID = 0;

BoneData::BoneData(std::string name): m_name(name), m_ID(m_commonID++)
{
}

BoneData::~BoneData()
{
	for (int i = 0; i < m_childs.size(); i++)
		delete m_childs[i];
}


void BoneData::addChild(BoneData* child)
{
	m_childs.push_back(child);
}

BoneData* BoneData::getChild(UINT id)
{
	if (id < m_childs.size())
		return m_childs[id];
	else return nullptr;
}

void BoneData::addAnimation(std::string animationName, BoneAnimation animation)
{
	m_animations[animationName] = animation;
	m_animations[animationName].evaluateBeginEndTime();
}

BoneAnimation* BoneData::getAnimation(std::string& animationName)
{
	auto it = m_animations.find(animationName);
	if (it != m_animations.end())
		return &(it->second);
	else
		return nullptr;
}

void BoneData::interpolate(float t, std::string& animationName)
{
	BoneAnimation* animation = getAnimation(animationName);
	if (animation)
	{
		animation->interpolate(t, m_toParentTransform);
	}

	for (int i = 0; i < m_childs.size(); i++)
		m_childs[i]->interpolate(t, animationName);
}

void BoneData::interpolateNode(float t, std::string& animationName) 
{
	BoneAnimation* animation = getAnimation(animationName);
	if (animation)
	{
		animation->interpolate(t, m_finalTransform);
	}	
}

void BoneData::buildToRoot(BoneData* parent)
{
	if (parent == nullptr)
	{
		m_toRootTransform = m_toParentTransform;
		XMMATRIX lBindMatrix = XMLoadFloat4x4(&m_bindTransform);
		XMMATRIX lToRoot = XMLoadFloat4x4(&m_toRootTransform);
		XMStoreFloat4x4(&m_finalTransform, XMMatrixMultiply(lBindMatrix, lToRoot));
		int a = 1;
	}

	else
	{
		XMMATRIX lToParent = XMLoadFloat4x4(&m_toParentTransform);
		XMMATRIX lParentToRoot = XMLoadFloat4x4(&parent->m_toRootTransform);
		XMMATRIX lBindMatrix = XMLoadFloat4x4(&m_bindTransform);

		XMMATRIX lToRoot = XMMatrixMultiply(lToParent, lParentToRoot);
		XMStoreFloat4x4(&m_toRootTransform, lToRoot);
		XMStoreFloat4x4(&m_finalTransform, XMMatrixMultiply(lBindMatrix, lToRoot));
		int a = 10;
	}
	
	for (int i = 0; i < m_childs.size(); i++)
		m_childs[i]->buildToRoot(this);
}

void BoneData::getFinalMatrices(std::vector<DirectX::XMFLOAT4X4>& finalMatrices)
{
	finalMatrices[m_ID] = m_finalTransform;
	for (int i = 0; i < m_childs.size(); i++)
		m_childs[i]->getFinalMatrices(finalMatrices);
}

DirectX::XMFLOAT4X4& BoneData::getFinalMatrix()
{
	return m_finalTransform;	
}

void BoneData::get_begin_end_animationTime(std::string& animationName, float& beginT, float& endT)
{
	BoneAnimation* ltAnimation = getAnimation(animationName);
	if (ltAnimation)
	{
		beginT = ltAnimation->getStartTime();
		endT = ltAnimation->getEndTime();
	}

	for (int i = 0; i < m_childs.size(); i++)
		m_childs[i]->get_begin_end_animationTime(animationName, beginT, endT);
}