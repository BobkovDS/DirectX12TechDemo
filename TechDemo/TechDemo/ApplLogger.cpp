#include "ApplLogger.h"
#include "ApplException.h"

using namespace std;

int AutoIncrementer::Shift = 0;

ApplLogger::ApplLogger(string fileName)
{
	m_oLogFile.open(fileName);

	if (!m_oLogFile)
		throw MyCommonRuntimeException("Error of creating lof file");

	m_oLogFile.setf(ios::unitbuf); // does not work ..hm
}

ApplLogger& ApplLogger::getLogger()
{
	return m_logger;
}

void ApplLogger::log(const string& msg, int shiftCount)
{	
	wstring lwStr(msg.begin(), msg.end());
	for (int i = 0; i < shiftCount; i++)
		m_oLogFile << "\t";

	m_oLogFile << lwStr.c_str() << endl <<flush; 
}

void ApplLogger::log(wstring& msg, int shiftCount)
{
	m_oLogFile << msg.c_str() << endl << flush;
}

ApplLogger::~ApplLogger()
{
}
