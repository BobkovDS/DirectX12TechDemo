#pragma once
#include "stdafx.h"

#define MAX_ANIM_TIME 1000000

struct KeyFrame {
	float TimePos;
	DirectX::XMFLOAT4X4 Transform; // to_Parent transform value at key time
	//or
	DirectX::XMFLOAT4 Translation;
	DirectX::XMFLOAT4 Scale;
	DirectX::XMFLOAT4 Rotation;
};

class BoneAnimation
{
	std::vector<KeyFrame> m_keyFrames;
	float m_beginTime;
	float m_endTime;	
public:
	BoneAnimation();
	~BoneAnimation();

	float getStartTime() const { return m_beginTime; };
	float getEndTime() const { return m_endTime; };	
	void evaluateBeginEndTime();
	void interpolate(float t, DirectX::XMFLOAT4X4& interpolatedValue) const;	
	void addKeyFrame(KeyFrame& keyFrame);
	KeyFrame& getKeyFrame(UINT i);
};

