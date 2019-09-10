#pragma once
#include "BasicDXGI.h"
#include "Utilit3D.h"
#include "Camera.h"
#include "FrameResourcesManager.h"
#include "ApplDataStructures.h"
#include "PSOLogoLayer.h"

#define PASSCONSTBUFCOUNT 3

class LogoAwaiter :
	public BasicDXGI
{		
	FrameResourcesManager<InstanceDataGPU, PassConstantsGPU, SSAO_GPU> m_frameResourceManager;
	Utilit3D m_utilit3D;
	Camera* m_camera;	
	Timer m_animationTimer;
	PSOLogoLayer m_psoLayer;
	std::unique_ptr<Mesh> m_logoMesh;

	float m_animTime;
	bool m_init3D_done;	

	void init3D();
	void update();
	void work();
	void onReSize(int newWidth, int newHeight);	
	void draw();
	void renderUI();

	void createLogoInformation();
public:
	LogoAwaiter(HINSTANCE hInstance, const std::wstring& applName, int width, int height);
	~LogoAwaiter();
};

