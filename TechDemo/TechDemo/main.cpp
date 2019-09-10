#include "TechDemo.h"
#include "ApplException.h"
#include "ApplLogger.h"
#include "LogoAwaiter.h"

using namespace std;

Canvas* Canvas::m_ptrThisCanvas = nullptr;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {

	ApplLogger::getLogger().log("Application begin to work");

	try
	{
		TechDemo my3DAppl(hInstance, L"my 3D application", 1000, 1000);
		
		//BasicDXGI my3DAppl(hInstance );
		my3DAppl.run();
	}
	catch (Canvas::CanvasException& exc)
	{
		ApplLogger::getLogger().log(exc.what());
		OutputDebugStringA(exc.what());
	}
	catch (MyCommonRuntimeException& runTimeException)
	{
		MessageBox(0, runTimeException.what().c_str(), runTimeException.where().c_str(), 0);
	}	

	ApplLogger::getLogger().log("Application finish the work\n");
	
	return 0;
}