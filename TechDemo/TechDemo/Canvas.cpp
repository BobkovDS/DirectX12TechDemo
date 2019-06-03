#include "Canvas.h"

using namespace std;// ::ostream;

Canvas::Canvas(HINSTANCE hInstance, const std::wstring& applName, int width , int height): m_hInstance(hInstance), m_width(width), m_height(height)
{
	oLogFile.open("output");
	
	if (!oLogFile)
		throw CanvasException("Canvas constructor: error of creating lof file");
	
	oLogFile.setf(ios::unitbuf); // does not work ..hm
	
	assert(m_ptrThisCanvas == 0);
	m_ptrThisCanvas = this;
	m_ApplicationName = applName;
	toLimitFPS = true;
	LOG("canvas was created");
}

Canvas::~Canvas()
{
	LOG("canvas was destroyed");	
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void Canvas::init() {

	LOG("Canvas::init()");
	
	WNDCLASS wc = {};

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc; 
	wc.hInstance = m_hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
	wc.lpszClassName = L"CanvasFor3DApplication";

	ATOM res = RegisterClass(&wc);
	assert(res !=0);

	RECT wrect = { 0,0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
	AdjustWindowRect(&wrect, WS_OVERLAPPEDWINDOW, false);

	m_hMainWind = CreateWindow(L"CanvasFor3DApplication", m_ApplicationName.c_str(), WS_OVERLAPPEDWINDOW, 100, 500,
		m_width, m_height, 0, 0, m_hInstance, 0);

	assert(m_hMainWind != 0);

	ShowWindow(m_hMainWind, SW_SHOW);
	UpdateWindow(m_hMainWind);
	m_windowWasCreated = true;
}

LRESULT Canvas::msgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			//PostMessage(hwnd, WM_DESTROY, 0, 0);
			PostQuitMessage(0);
			return 0;
		}
		if (wParam == VK_SPACE)
		{
			m_timer.tt_Pause();
			onKeyDown(wParam);
		}
		else onKeyDown(wParam);
		return 0;

	case WM_KEYUP:
		onKeyUp(wParam);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		onMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		onMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_MOUSEMOVE:
		onMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_SIZE:

		onReSize(LOWORD(lParam), HIWORD(lParam));		
		return 0;		
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void Canvas::run() {
	init(); // initialization to create window
	init3D(); //initialization to create 3D application
	
	m_timer.tt_RunStop();
	m_FPSCount = 0;
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			double frameTimeForLimitFPS = 0.008333; //for 120fps;

			m_timer.begin();
			work();
			double longTimeInSec = m_timer.end();
			m_FPSCount++;
			if (toLimitFPS) // if we want limit FPS to 120
			{
				double howLongToWait = 0;
				if (longTimeInSec < frameTimeForLimitFPS)
					howLongToWait = (frameTimeForLimitFPS - longTimeInSec) * 1000;
					Sleep(howLongToWait);
			}

			string test("FPS: ");
			if (m_timer.tick1Sec())
			{					
				m_FPSCountOut = m_FPSCount;
				m_FPSCount = 0;
			}	
			test += std::to_string(m_FPSCountOut) + " limitFPS(" + std::to_string(toLimitFPS) + ")" +
				" timer: " + std::to_string(m_timer.totalTime());

			test += addTextToWindow();
			SetWindowTextA(m_hMainWind, test.c_str());
		}
	}	
}

void Canvas::onReSize(int newWidth, int newHeight) {
	m_width = newWidth;
	m_height = newHeight;
}

Canvas* Canvas::getCanvas() {
	return m_ptrThisCanvas;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return Canvas::getCanvas()->msgProc(hwnd, msg, wParam, lParam);
}

