#include "SkeletonManager.h"



SkeletonManager::SkeletonManager()
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