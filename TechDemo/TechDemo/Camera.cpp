#include "Camera.h"
using namespace DirectX;

Camera::Camera(): m_viewToUpdate(false)
{
	m_position = { 0.0f, 0.0f, 0.0f };
	m_right = { 1.0f, 0.0f, 0.0f };
	m_up = { 0.0f, 1.0f, 0.0f };
	m_look = { 0.0f, 0.0f, 1.0f };	

	lens = new PerspectiveCameraLens();
}

//copy constructor
Camera& Camera::operator=(Camera& a)
{
	m_position = a.m_position;
	m_right = a.m_right;
	m_up = a.m_up;
	m_look = a.m_look;
	m_viewToUpdate = a.m_viewToUpdate;
	m_boundingFrustumWorldToUpdate = a.m_boundingFrustumWorldToUpdate;
	m_viewMatrix = a.m_viewMatrix;	
	m_observers = a.m_observers;
	*lens = *a.lens;

	return *this;
}

Camera::~Camera()
{	
	delete lens;
	lens = 0;

	for (int i = 0; i < m_observers.size(); i++)
		delete m_observers[0];
}

void Camera::updateViewMatrix()
{
	/*
		Calculate a View matrix - for transformation from World space to View space (Camera space)
		for this we need to know Camera position, Up vector and targent point
	*/

	if (!m_viewToUpdate) return; //nothing to update
	m_viewToUpdate = false;
	
	XMVECTOR R = XMLoadFloat3(&m_right);
	XMVECTOR U = XMLoadFloat3(&m_up);
	XMVECTOR L = XMLoadFloat3(&m_look);
	XMVECTOR P = XMLoadFloat3(&m_position);	
	
	// Keep camera's axes orthogonal to each other and of unit lenght
	L = XMVector3Normalize(L);
	U = XMVector3Normalize(XMVector3Cross(L,R));
	R = XMVector3Cross(U, L);	

	XMStoreFloat3(&m_right, R);
	XMStoreFloat3(&m_up, U);
	XMStoreFloat3(&m_look, L);

	float x = -XMVectorGetX(XMVector3Dot(P, R));
	float y = -XMVectorGetX(XMVector3Dot(P, U));
	float z = -XMVectorGetX(XMVector3Dot(P, L));
	
	m_viewMatrix(0, 0) = m_right.x;
	m_viewMatrix(1, 0) = m_right.y;
	m_viewMatrix(2, 0) = m_right.z;
	m_viewMatrix(3, 0) = x;

	m_viewMatrix(0, 1) = m_up.x;
	m_viewMatrix(1, 1) = m_up.y;
	m_viewMatrix(2, 1) = m_up.z;
	m_viewMatrix(3, 1) = y;

	m_viewMatrix(0, 2) = m_look.x;
	m_viewMatrix(1, 2) = m_look.y;
	m_viewMatrix(2, 2) = m_look.z;
	m_viewMatrix(3, 2) = z;
	
	m_viewMatrix(0, 3) = 0.0f;
	m_viewMatrix(1, 3) = 0.0f;
	m_viewMatrix(2, 3) = 0.0f;
	m_viewMatrix(3, 3) = 1.0f;	
	
	updateObservers();
	m_boundingFrustumWorldToUpdate = true;
}

void Camera::lookTo(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR lookDir, DirectX::FXMVECTOR worldUp)
{	
	XMVECTOR L = XMVector3Normalize(lookDir);	
	L = XMVectorSet(XMVectorGetX(L), XMVectorGetY(L), XMVectorGetZ(L), 0.0f);
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&m_position, pos);
	XMStoreFloat3(&m_right, R);
	XMStoreFloat3(&m_up, U);
	XMStoreFloat3(&m_look, L);

	m_viewToUpdate = true;
}

void Camera::lookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp)
{
	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
	XMVECTOR U = XMVector3Cross(L, R);	

	XMStoreFloat3(&m_position, pos);
	XMStoreFloat3(&m_right, R);
	XMStoreFloat3(&m_up, U);	
	XMStoreFloat3(&m_look, L);

	m_viewToUpdate = true;
}

void Camera::lookAt(DirectX::XMFLOAT3 ipos, DirectX::XMFLOAT3 itarget, DirectX::XMFLOAT3 iworldUp)
{
	XMVECTOR position = XMVectorSet(ipos.x, ipos.y, ipos.z, 1.0f);
	XMVECTOR target = XMVectorSet(itarget.x, itarget.y, itarget.z, 1.0f);
	XMVECTOR worldUp = XMVectorSet(iworldUp.x, iworldUp.y, iworldUp.z, 0.0f);
	
	lookAt(position, target, worldUp);
}

DirectX::XMMATRIX Camera::getView()
{
	updateViewMatrix();
	return DirectX::XMLoadFloat4x4(&m_viewMatrix);	
}

const DirectX::XMFLOAT4X4& Camera::getProj4x4f()
{
	return lens->getProj4x4f();
}

void Camera::strafe(float d)
{
	XMVECTOR S = XMVectorReplicate(d);
	XMVECTOR P = XMLoadFloat3(&m_position);
	XMVECTOR R = XMLoadFloat3(&m_right);	
	XMStoreFloat3(&m_position, XMVectorMultiplyAdd(S,R,P));

	m_viewToUpdate = true;
}

void Camera::walk(float d)
{	
	XMVECTOR S = XMVectorReplicate(d);
	XMVECTOR P = XMLoadFloat3(&m_position);
	XMVECTOR L = XMLoadFloat3(&m_look);
	XMStoreFloat3(&m_position, XMVectorMultiplyAdd(S, L, P));

	m_viewToUpdate = true;
}

void Camera::pitch(float angle)
{	
	// Rotate up and look vectors about the right vector
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&m_right), angle);

	XMStoreFloat3(&m_up, XMVector3TransformNormal(XMLoadFloat3(&m_up), R));
	XMStoreFloat3(&m_look, XMVector3TransformNormal(XMLoadFloat3(&m_look), R));

	m_viewToUpdate = true;
}

void Camera::rotateY(float angle)
{
	//Rotate the basis vectors about the world y-axis
	XMMATRIX R = XMMatrixRotationY(angle);

	XMStoreFloat3(&m_right, XMVector3TransformNormal(XMLoadFloat3(&m_right), R));
	XMStoreFloat3(&m_up, XMVector3TransformNormal(XMLoadFloat3(&m_up), R));
	XMStoreFloat3(&m_look, XMVector3TransformNormal(XMLoadFloat3(&m_look), R));

	m_viewToUpdate = true;
}

/*Transform camera with animation transform matrix*/
void Camera::transform(XMFLOAT4X4& transformM)
{
	XMMATRIX lAnimTransform = XMLoadFloat4x4(&transformM);
	
	if (XMMatrixIsIdentity(lAnimTransform)) return;	

	XMVECTOR lPos = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR lLook = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);	
	
	lPos = XMVector3Transform(lPos, lAnimTransform);
	lLook = XMVector3TransformNormal(lLook, lAnimTransform);	

	lookTo(lPos, lLook, XMVectorSet(0.0f, 1.0f, 0.0, 0.0f));
}

const DirectX::XMFLOAT3& Camera::getPosition3f()
{
	return m_position;
}

inline void Camera::setPosition(float x, float y, float z)
{
	m_position = XMFLOAT3(x, y, z);
}

inline void Camera::setPosition(const DirectX::XMFLOAT3& v)
{
	m_position = v;
}

inline XMVECTOR Camera::getRight() const
{
	return XMLoadFloat3(&m_right);
}
inline const XMFLOAT3& Camera::getRight3f()
{
	return m_right;
}

inline XMVECTOR Camera::getUp() const
{
	return XMLoadFloat3(&m_up);
}

const XMFLOAT3& Camera::getUp3f()
{
	return m_up;
}

const XMFLOAT3& Camera::getLook3f()
{
	return m_look;
}

XMFLOAT3 Camera::getTarget3f()
{
	XMFLOAT3 target;
	XMVECTOR S = XMVectorReplicate(1.0f);
	XMVECTOR P = XMLoadFloat3(&m_position);
	XMVECTOR L = XMLoadFloat3(&m_look);
	XMStoreFloat3(&target, XMVectorMultiplyAdd(S, L, P));

	return target;
}

/*Get BoundingFrustum for Frustum culling calculation. */
BoundingMath::BoundingFrustum& Camera::getBoundingFrustomCameraWorld()
{
	updateViewMatrix();

	if (m_boundingFrustumWorldToUpdate) // it means ViewMatrix was updated, so lets update BoundingFrustum too
	{
		DirectX::XMMATRIX view = XMLoadFloat4x4(&m_viewMatrix);
		DirectX::XMMATRIX proj = lens->getProj();
		DirectX::XMMATRIX cb = XMMatrixMultiply(view, proj);		
		
		BoundingMath::BoundingFrustum::getPlanesFromMatrix(m_boundingFrustumCameraWorld, cb);

		m_boundingFrustumWorldToUpdate = false;
	}
	return m_boundingFrustumCameraWorld;
}

inline void Camera::updateObservers()
{
	for (int i = 0; i < m_observers.size(); i++)
		m_observers[0]->execute();	
}

// --------------------------------------------						----------------------------------
// -------------------------------------------- CAMERA LENS CLASSES ----------------------------------
// --------------------------------------------						----------------------------------


inline DirectX::XMMATRIX Camera::CameraLens::getProj() const
{
	assert(m_lensWasSet);
	return DirectX::XMLoadFloat4x4(&m_projectionMatrix);
}

inline const DirectX::XMFLOAT4X4& Camera::CameraLens::getProj4x4f()
{
	assert(m_lensWasSet);
	return m_projectionMatrix;
}

PerspectiveCameraLens::PerspectiveCameraLens():
	m_nearZ(1), 
	m_farZ(100),
	m_aspect(0),
	m_fovY(0), 	
m_nearWindowHeight(0), m_farWindowHeight(0)
{	
}

void PerspectiveCameraLens::_copy(const CameraLens* _in, CameraLens* _out)
{
	const PerspectiveCameraLens* inLensData = dynamic_cast< const PerspectiveCameraLens*>(_in);
	PerspectiveCameraLens* outLensData = dynamic_cast<PerspectiveCameraLens*>(_out);
	
	outLensData->m_nearZ = inLensData->m_nearZ;
	outLensData->m_farZ = inLensData->m_farZ;
	outLensData->m_aspect = inLensData->m_aspect;
	outLensData->m_fovY = inLensData->m_fovY;
	outLensData->m_nearWindowHeight = inLensData->m_nearWindowHeight;
	outLensData->m_farWindowHeight = inLensData->m_farWindowHeight;
}


void PerspectiveCameraLens::setLens(float fovY, float aspect, float zn, float zf)
{
	m_fovY = fovY;
	m_aspect = aspect;
	m_nearZ = zn;
	m_farZ = zf;

	m_nearWindowHeight = 2.0f * m_nearZ * tanf(0.5f*m_fovY);
	m_farWindowHeight = 2.0f * m_farZ * tanf(0.5f*m_fovY);

	XMMATRIX P = XMMatrixPerspectiveFovLH(m_fovY, m_aspect, m_nearZ, m_farZ);
	XMStoreFloat4x4(&m_projectionMatrix, P);
		
	m_lensWasSet = true;
}

void PerspectiveCameraLens::setAspectRatio(float aspectRatio)
{
	setLens(m_fovY, aspectRatio, m_nearZ, m_farZ);	
}

// ------------ Orthographic Camera Lens --------------------------------

OrthographicCameraLesn& OrthographicCameraLesn::operator=(const OrthographicCameraLesn& c)
{
	m_projectionMatrix = c.m_projectionMatrix;
	return *this;
}

void OrthographicCameraLesn::setLens(float x, float y, float z, float radius)
{	
	XMMATRIX P = XMMatrixOrthographicLH(10, 10, 1.0f, 100.0f);	
	XMStoreFloat4x4(&m_projectionMatrix, P);
	m_lensWasSet = true;
}

void OrthographicCameraLesn::setLens(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 extents)
{
	float l = center.x - extents.x;
	float r = center.x + extents.x;

	float b = center.y - extents.y;
	float t = center.y + extents.y;

	float n = 0.0f;// extents.z;// center.z - extents.z;
	float f = n + extents.z*2;

	XMMATRIX P = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);
	XMStoreFloat4x4(&m_projectionMatrix, P);
	m_lensWasSet = true;
}