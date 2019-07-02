#include "SkeletonManager.h"



SkeletonManager::SkeletonManager(): m_animationTimeWasEvaluated(false)
{
}


SkeletonManager::~SkeletonManager()
{
}

//SkinnedData* SkeletonManager::addSkeleton(std::string skeletonName)
//{
//	//we do not need it maybe
//	return nullptr;
//}


SkinnedData& SkeletonManager::getSkeletonSkinnedAnimated(std::string& skeletonName)
{
	return m_skeletons_SkinnedAnimated[skeletonName]; // it will be created, if we do not have this skeleton yet
}

SkinnedData& SkeletonManager::getSkeletonSkinnedAnimated(UINT id)
{
	auto it_begin = m_skeletons_SkinnedAnimated.begin();
	if (id > m_skeletons_SkinnedAnimated.size()) id = m_skeletons_SkinnedAnimated.size();

	for (int i = 0; i < id; i++)
		it_begin++;

	return it_begin->second;
}

int SkeletonManager::getSkeletonSkinnedAnimatedCount()
{
	return m_skeletons_SkinnedAnimated.size();
}

SkinnedData& SkeletonManager::getSkeletonNodeAnimated(std::string& skeletonName)
{
	return m_skeletons_NodeAnimated[skeletonName]; // it will be created, if we do not have this skeleton yet
}

bool SkeletonManager::isExistSkeletonNodeAnimated(std::string& skeletonName)
{
	auto it = m_skeletons_NodeAnimated.find(skeletonName);
	return (it != m_skeletons_NodeAnimated.end());
}

SkinnedData& SkeletonManager::getSkeletonNodeAnimated(UINT id)
{
	auto it_begin = m_skeletons_NodeAnimated.begin();
	if (id > m_skeletons_NodeAnimated.size()) id = m_skeletons_NodeAnimated.size();

	for (int i = 0; i < id; i++)
		it_begin++;

	return it_begin->second;
}

int SkeletonManager::getSkeletonNodeAnimatedCount()
{
	return m_skeletons_NodeAnimated.size();
}

void SkeletonManager::evaluateAnimationsTime()
{
	{
		auto begin_it = m_skeletons_SkinnedAnimated.begin();
		for (; begin_it != m_skeletons_SkinnedAnimated.end(); begin_it++)
			begin_it->second.evaluateBeginEndTime();
	}

	{
		auto begin_it = m_skeletons_NodeAnimated.begin();
		for (; begin_it != m_skeletons_NodeAnimated.end(); begin_it++)
			begin_it->second.evaluateBeginEndTime();
	}
	m_animationTimeWasEvaluated = false;
}

void SkeletonManager::getAnimationsTime(float& beginT, float& endT)
{
	assert(m_animationTimeWasEvaluated);

	//auto begin_it = m_skeletons.begin();
	//for (; begin_it != m_skeletons.end(); begin_it++)
	//	begin_it->second.
}