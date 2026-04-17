#pragma once

#include <fstream>
#include <memory>

#include "global.h"

class Logger
{
public:
	enum ETextType
	{
		ETextTypeDefault,
		ETextTypeWarning,
		ETextTypeError,
		ETextTypeSuccess
	};

protected:
	std::ofstream m_file;

	static std::unique_ptr<Logger> m_instance;
	static void LogImpl(const std::string& strMessage, unsigned int iStep = 0, ETextType eType = ETextType::ETextTypeDefault);

public:
	Logger();

	static bool InitInstance();

	static void Log(const std::string& strMessage, unsigned int iStep = 0);
	static void Success(const std::string& strMessage, unsigned int iStep = 0);
	static void Warning(const std::string& strMessage, unsigned int iStep = 0);
	static void Error(const std::string& strMessage, unsigned int iStep = 0);
	static void PrintProgress(const std::string& strPrefix, int iCount, int iMaxCount);
};