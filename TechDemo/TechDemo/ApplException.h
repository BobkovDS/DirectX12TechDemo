#pragma once
#include <stdexcept>

class MyCommonRuntimeException : public std::runtime_error {
	std::wstring m_errorMsg;
	std::wstring m_where;
public:
	MyCommonRuntimeException(const std::string& errMsg = "commonRuntimeException", const std::wstring& fromWhere = L"none") :
		runtime_error(errMsg), m_errorMsg(L"="), m_where(fromWhere) {}
	MyCommonRuntimeException(const std::wstring& errMsg = L"commonRuntimeException_wchar", const std::wstring& fromWhere = L"none") :
		runtime_error("Error WCHAR"), m_errorMsg(errMsg), m_where(fromWhere)
	{}

	std::wstring what()
	{
		if (m_errorMsg != L"=")
			return m_errorMsg;
		else
		{
			std::string tmpError = runtime_error::what();
			std::wstring result(tmpError.begin(), tmpError.end());
			return result;
		}
	}

	std::wstring& where()
	{
		m_where.insert(0, L"Runtime error: ");

		return m_where;
	}
};
