#include "BoneAnimation.h"

using namespace DirectX;

BoneAnimation::BoneAnimation()
{
}


BoneAnimation::~BoneAnimation()
{
}

void BoneAnimation::addKeyFrame(KeyFrame& keyFrame)
{
	m_keyFrames.push_back(keyFrame);
}

KeyFrame& BoneAnimation::getKeyFrame(UINT i)
{
	if (i < m_keyFrames.size())
		return m_keyFrames[i];
	else
		return m_keyFrames.back();
}

float BoneAnimation::getEndTime() const
{
	//if (m_keyFrames.size() == 0) return 0;
	return m_keyFrames.back().TimePos;
}

float BoneAnimation::getStartTime()	const
{
	return m_keyFrames.front().TimePos;
}

void BoneAnimation::interpolate(float t, XMFLOAT4X4& interpolatedValue) const
{
	if (t < getStartTime())
		interpolatedValue = m_keyFrames.front().Transform;
	else
		if (t>getEndTime())
		interpolatedValue = m_keyFrames.back().Transform;
		else
		{
			for (int i=0; i<m_keyFrames.size()-1; i++)
				if (t >= m_keyFrames[i].TimePos && t < m_keyFrames[i + 1].TimePos)
				{
					float lLerpPercent = (t - m_keyFrames[i].TimePos) / (m_keyFrames[i+1].TimePos - m_keyFrames[i].TimePos);
					
					// interpolation job here
					XMVECTOR s0 = XMLoadFloat4(&m_keyFrames[i].Scale);
					XMVECTOR s1 = XMLoadFloat4(&m_keyFrames[i+1].Scale);

					XMVECTOR t0 = XMLoadFloat4(&m_keyFrames[i].Translation);
					XMVECTOR t1 = XMLoadFloat4(&m_keyFrames[i+1].Translation);

					XMVECTOR r0 = XMLoadFloat4(&m_keyFrames[i].Rotation);
					XMVECTOR r1 = XMLoadFloat4(&m_keyFrames[i + 1].Rotation);

					XMVECTOR S = XMVectorLerp(s0, s1, lLerpPercent);
					XMVECTOR T = XMVectorLerp(t0, t1, lLerpPercent);
					XMVECTOR R = XMVectorLerp(r0, r1, lLerpPercent);

					XMMATRIX Sm = XMMatrixScalingFromVector(S);
					XMMATRIX Tm = XMMatrixScalingFromVector(T);
					XMMATRIX Rm = XMMatrixScalingFromVector(R);

					XMMATRIX C = Sm * Tm * Rm;
					XMStoreFloat4x4(&interpolatedValue, C);
				}
		}
}