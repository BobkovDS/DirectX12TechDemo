#pragma once
#include <windows.h>
#include <windowsx.h>
#include <cassert>
#include <string>
#include <stdexcept>
#include <fstream>
#include "Timer.h"

class Canvas
{
	bool m_timerPause;
	Timer m_timer;
	int m_FPSCount;
	int m_FPSCountOut;
private:	
	static Canvas* m_ptrThisCanvas; // canvas can be only one. And we keep value for it for callback function WindowProc
	HINSTANCE m_hInstance;
	HWND m_hMainWind;
	int m_width;
	int m_height;
	RECT m_rectBeforeFullScreen;
	std::wstring m_ApplicationName;	
	bool m_windowWasCreated;
	bool m_isFullScreen;
	
	void init();	
	
	virtual LRESULT msgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); // func1
	static Canvas* getCanvas();													// func2		
	friend LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); /* to give access to func1 and func2
	functions if want to keep it private*/
	
	Canvas(const Canvas&) {}; // to use copy constructor is prohibited
	void operator=(const Canvas&) {}; // to use operator= is prohibited
protected:
	std::wofstream oLogFile;
	bool toLimitFPS;	
	void toggleFullScreen();
	virtual void onMouseDown(WPARAM btnState, int x, int y) {}
	virtual void onMouseUp(WPARAM btnState, int x, int y) {}
	virtual void onMouseMove(WPARAM btnState, int x, int y) {}
	virtual void onKeyDown(WPARAM btnState) {}
	virtual void onKeyUp(WPARAM btnState) {}
	virtual void onReSize(int newWidth, int newHeight);	

	virtual void init3D() { } // give a chance for inherited class to call his Initialization indepedently
	virtual void work() { } // will give a chance to other class to do usefull job in main circle of application, in "run" function
	virtual std::string addTextToWindow() { return ""; };
	UINT width() const { return m_width; }
	UINT height() const { return m_height; }
	HWND hMainWind() const {return m_hMainWind; }	

public:
	Canvas(HINSTANCE hInstance, const std::wstring& applName, int width=500, int heigh=500);
	virtual ~Canvas();
	
	class CanvasException : public std::logic_error {
	public:
		CanvasException(const char* msg) : logic_error(msg) {}
	};

	bool windowWasCreated() const { return m_windowWasCreated; }
	void run();
};

#define LOG(A) oLogFile<< A << endl<< flush; // flush doesnt work
