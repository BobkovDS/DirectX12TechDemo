#pragma once
#include "BasicDXGI.h"
class TechDemo :
	public BasicDXGI
{
public:
	TechDemo(HINSTANCE hInstance, const std::wstring& applName, int width, int height);
	~TechDemo();
};

