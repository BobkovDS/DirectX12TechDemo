#include "SkinnedData.h"

SkinnedData::SkinnedData():m_animationTime(0)
{
	DirectX::XMStoreFloat4x4(&m_identityMatrix, DirectX::XMMatrixIdentity());		
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

void SkinnedData::getMinMaxBounaryTimeValues(float& bt, float& et)
{
	bt = m_beginTimeMinForAllAnimations;
	et = m_endTimeMaxForAllAnimations;
}

void SkinnedData::addAnimationName(std::string animName)
{
	m_animations.push_back(animName);
}

void SkinnedData::evaluateBeginEndTime()
{
	/*
		This method should be called at the end, when we have all Animations added;
	*/

	// This min and max value for Begin and End time through all Animations
	m_beginTimeMinForAllAnimations = MAX_ANIM_TIME;
	m_endTimeMaxForAllAnimations = 0;

	for (int i = 0; i < m_animations.size(); i++)
	{
		for (int ri = 0; ri < m_root_bones.size(); ri++)
		{
			// Begin and End time for specific animation
			float lBeginTime = MAX_ANIM_TIME;
			float lEndTime = 0;
			m_root_bones[ri]->get_begin_end_animationTime(m_animations[i], lBeginTime, lEndTime);

			if (lBeginTime < m_beginTimeMinForAllAnimations) m_beginTimeMinForAllAnimations = lBeginTime;
			if (lEndTime < m_endTimeMaxForAllAnimations) m_endTimeMaxForAllAnimations = lEndTime;

			if (lBeginTime == MAX_ANIM_TIME && lEndTime == 0)			
				lBeginTime = 0;			
			
			m_animationsBeginEndTimes[m_animations[i]] = std::make_pair(lBeginTime, lEndTime);
		}
	}
	// no animations at all or something wrong with animation time
	if (m_beginTimeMinForAllAnimations == MAX_ANIM_TIME && m_endTimeMaxForAllAnimations == 0)
		m_beginTimeMinForAllAnimations = 0;
}

void SkinnedData::interpolate(float t, UINT animationID)
{	
	if (animationID < m_animations.size())	
	{
		float lAnimBeginTime = m_animationsBeginEndTimes[m_animations[animationID]].first;
		float lAnimEndTime = m_animationsBeginEndTimes[m_animations[animationID]].second;

		if (m_animationTime > lAnimEndTime)
			m_animationTime = lAnimBeginTime;

		for (int i = 0; i < m_root_bones.size(); i++)
			m_root_bones[i]->interpolate(m_animationTime, m_animations[animationID]);
	}

	if (m_bonesFinalTransforms.size() < BoneData::getCommonIDValue()) //if we have more bones then we expect
		m_bonesFinalTransforms.resize(BoneData::getCommonIDValue());

	for (int i = 0; i < m_root_bones.size(); i++)
		m_root_bones[i]->buildToRoot(NULL);

	for (int i = 0; i < m_root_bones.size(); i++)
		m_root_bones[i]->getFinalMatrices(m_bonesFinalTransforms);
}

const std::vector<DirectX::XMFLOAT4X4>& SkinnedData::getFinalTransforms(float dt, UINT animationID)
{
	m_animationTime += dt;
	interpolate(m_animationTime, animationID);

	return m_bonesFinalTransforms;
}

DirectX::XMFLOAT4X4& SkinnedData::getNodeTransform(float dt, UINT animationID)
{
	m_animationTime += dt;
	
	if (animationID < m_animations.size())
	{
		float lAnimBeginTime = m_animationsBeginEndTimes[m_animations[animationID]].first;
		float lAnimEndTime = m_animationsBeginEndTimes[m_animations[animationID]].second;

		if (m_animationTime > lAnimEndTime)
			m_animationTime = lAnimBeginTime;

		assert(m_root_bones.size() == 1);// we think that for NodeAnimated object should be 1 root bone
		
		m_root_bones[0]->interpolateNode(m_animationTime, m_animations[animationID]);
			
		return m_root_bones[0]->getFinalMatrix();		
	}
	else
		return m_identityMatrix;

	
}