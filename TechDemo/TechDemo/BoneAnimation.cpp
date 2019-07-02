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

void BoneAnimation::evaluateBeginEndTime()
{
	m_beginTime = MAX_ANIM_TIME;
	m_endTime = 0;

	for (int i = 0; i < m_keyFrames.size(); i++)
	{
		if (m_keyFrames[i].TimePos < m_beginTime) m_beginTime = m_keyFrames[i].TimePos;
		if (m_keyFrames[i].TimePos > m_endTime) m_endTime= m_keyFrames[i].TimePos;
	}
}
void BoneAnimation::interpolate(float time, XMFLOAT4X4& interpolatedValue) const
{
	KeyFrame lKeyFrame = {};
	if (time < getStartTime())
		lKeyFrame = m_keyFrames.front();
	else
		if (time >getEndTime())
			lKeyFrame = m_keyFrames.back();
		else
		{
			for (int i=0; i<m_keyFrames.size()-1; i++)
				if (time >= m_keyFrames[i].TimePos && time < m_keyFrames[i + 1].TimePos)
				{
					float lLerpPercent = (time- m_keyFrames[i].TimePos) / (m_keyFrames[i+1].TimePos - m_keyFrames[i].TimePos);
						
					// interpolation job here
					XMVECTOR s0 = XMLoadFloat4(&m_keyFrames[i].Scale);
					XMVECTOR s1 = XMLoadFloat4(&m_keyFrames[i+1].Scale);

					XMVECTOR t0 = XMLoadFloat4(&m_keyFrames[i].Translation);
					XMVECTOR t1 = XMLoadFloat4(&m_keyFrames[i+1].Translation);

					XMVECTOR r0 = XMLoadFloat4(&m_keyFrames[i].Rotation);
					XMVECTOR r1 = XMLoadFloat4(&m_keyFrames[i + 1].Rotation);

					XMVECTOR s = XMVectorLerp(s0, s1, lLerpPercent);
					XMVECTOR t = XMVectorLerp(t0, t1, lLerpPercent);
					XMVECTOR r = XMVectorLerp(r0, r1, lLerpPercent);

					XMMATRIX Sm = XMMatrixScalingFromVector(s);
					XMMATRIX Tm = XMMatrixTranslationFromVector(t);					
					XMMATRIX Rxm = XMMatrixRotationX(XMVectorGetX(r));
					XMMATRIX Rym = XMMatrixRotationY(XMVectorGetY(r));
					XMMATRIX Rzm = XMMatrixRotationZ(XMVectorGetZ(r));
					XMMATRIX Rm = Rxm * Rym * Rzm;				

					XMMATRIX C = Sm * Rm *Tm;									
					//C = XMLoadFloat4x4(&m_keyFrames[i].Transform); // TEST
					XMStoreFloat4x4(&interpolatedValue, C);

					return;
				}
		}

	// interpolation job here for bound-keys
	XMVECTOR s = XMLoadFloat4(&lKeyFrame.Scale);
	XMVECTOR t = XMLoadFloat4(&lKeyFrame.Translation);
	XMVECTOR r = XMLoadFloat4(&lKeyFrame.Rotation);

	XMMATRIX Sm = XMMatrixScalingFromVector(s);
	XMMATRIX Tm = XMMatrixTranslationFromVector(t);
	XMMATRIX Rxm = XMMatrixRotationX(XMVectorGetX(r));
	XMMATRIX Rym = XMMatrixRotationY(XMVectorGetY(r));
	XMMATRIX Rzm = XMMatrixRotationZ(XMVectorGetZ(r));
	XMMATRIX Rm = Rxm * Rym * Rzm;

	XMMATRIX C = Sm * Rm *Tm;	
	XMStoreFloat4x4(&interpolatedValue, C);
	
}