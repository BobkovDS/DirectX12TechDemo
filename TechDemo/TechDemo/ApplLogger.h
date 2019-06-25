#pragma once
#include <fstream>

class ApplLogger
{
	static ApplLogger m_logger;	
	std::wofstream m_oLogFile;	
	ApplLogger(std::string fileName);

	ApplLogger& operator=(ApplLogger&) = delete;
	ApplLogger(const ApplLogger&) = delete;
public:
	static ApplLogger& getLogger();
	void log(const std::string& msg, int shiftCount=0);
	void log(std::wstring& msg, int shiftCount = 0);

	~ApplLogger();
};

