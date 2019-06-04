#include "TechDemo.h"
#include "ApplException.h"
#include <iostream>
using namespace std;

Canvas* Canvas::m_ptrThisCanvas = nullptr;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {

	cout << "created\n";

	try
	{
		TechDemo my3DAppl(hInstance, L"my 3D application", 1000, 1000);
		//BasicDXGI my3DAppl(hInstance );
		my3DAppl.run();
	}
	catch (Canvas::CanvasException& exc)
	{
		cout << exc.what();
		OutputDebugStringA(exc.what());
	}
	catch (MyCommonRuntimeException& runTimeException)
	{
		MessageBox(0, runTimeException.what().c_str(), runTimeException.where().c_str(), 0);
	}
	return 0;
}