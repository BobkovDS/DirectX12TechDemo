#pragma once
#include "SkinnedData.h"
#include <map>

class SkeletonManager
{
	std::map<std::string, SkinnedData> m_skeletons_SkinnedAnimated; // Skeletons for Skinned-animated object (Wolf)
	std::map<std::string, SkinnedData> m_skeletons_NodeAnimated; // skeletons for animated Objects like Camera or Light
	bool m_animationTimeWasEvaluated;
public:	

	SkeletonManager();
	~SkeletonManager();

	//SkinnedData* addSkeleton(std::string& skeletonName); /*TO_DO: delete*/
	SkinnedData& getSkeletonSkinnedAnimated(std::string& skeletonName);
	SkinnedData& getSkeletonSkinnedAnimated(UINT i);
	SkinnedData& getSkeletonNodeAnimated(std::string& skeletonName);
	SkinnedData& getSkeletonNodeAnimated(UINT i);

	int getSkeletonSkinnedAnimatedCount();
	int getSkeletonNodeAnimatedCount();

	void getAnimationsTime(float& beginT, float& endT);
	void evaluateAnimationsTime();
};

