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

void BoneData::buildToRoot(BoneData* parent)
{
	if (parent == nullptr)
		m_toRootTransform = m_toParentTransform;
	else
	{
		XMMATRIX lToParent = XMLoadFloat4x4(&m_toParentTransform);
		XMMATRIX lParentToRoot = XMLoadFloat4x4(&parent->m_toRootTransform);
		XMMATRIX lBindMatrix = XMLoadFloat4x4(&m_bindTransform);

		XMMATRIX lToRoot = XMMatrixMultiply(lParentToRoot, lToParent);
		XMStoreFloat4x4(&m_toRootTransform, lToRoot);
		XMStoreFloat4x4(&m_finalTransform, XMMatrixMultiply(lToRoot, lBindMatrix));
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