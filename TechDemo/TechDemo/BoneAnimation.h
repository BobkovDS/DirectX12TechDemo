#pragma once
#include "stdafx.h"

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
public:
	BoneAnimation();
	~BoneAnimation();

	float getStartTime() const;
	float getEndTime() const;
	void interpolate(float t, DirectX::XMFLOAT4X4& interpolatedValue) const;
	void addKeyFrame(KeyFrame& keyFrame);
	KeyFrame& getKeyFrame(UINT i);
};

