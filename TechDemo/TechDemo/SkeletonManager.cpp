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


SkinnedData& SkeletonManager::getSkeleton(std::string& skeletonName)
{
	return m_skeletons[skeletonName]; // it will be created, if we do not have this skeleton yet
}

int SkeletonManager::getSkeletonCount()
{
	return m_skeletons.size();
}

SkinnedData& SkeletonManager::getSkeleton(UINT id)
{
	auto it_begin = m_skeletons.begin();
	if (id > m_skeletons.size()) id = m_skeletons.size();

	for (int i = 0; i < id; i++)
		it_begin++;

	return it_begin->second;
}

void SkeletonManager::evaluateAnimationsTime()
{
	auto begin_it = m_skeletons.begin();
	for (; begin_it != m_skeletons.end(); begin_it++)
		begin_it->second.evaluateBeginEndTime();

	m_animationTimeWasEvaluated = false;
}

void SkeletonManager::getAnimationsTime(float& beginT, float& endT)
{
	assert(m_animationTimeWasEvaluated);

	//auto begin_it = m_skeletons.begin();
	//for (; begin_it != m_skeletons.end(); begin_it++)
	//	begin_it->second.
}