#include "CLogger.h"

#include <iostream>
#include <syncstream>

std::unique_ptr<Logger> Logger::m_instance{nullptr};

std::map<Logger::ETextType, const char*> s_mapColors
{
	{Logger::ETextTypeDefault,	"\033[0m"},
	{Logger::ETextTypeError,	"\033[31m"},
	{Logger::ETextTypeSuccess,	"\033[32m"},
	{Logger::ETextTypeWarning,	"\033[33m"},
};


Logger::Logger()
{
	auto path_to_log = GetWorkingDirectory() / "log" / "log.txt";

	if (!std::filesystem::exists(path_to_log.parent_path()))
		std::filesystem::create_directories(path_to_log.parent_path());

	m_file = std::ofstream{ path_to_log };
}//Logger::Logger()


bool Logger::InitInstance()
{
	if (!m_instance)
		m_instance = std::make_unique<Logger>();

	return m_instance && m_instance->m_file.is_open();
}//Logger::InitInstance()


void Logger::LogImpl(const std::string& strMessage, unsigned int iStep, ETextType eType)
{
	if (iStep)
		m_instance->m_file << std::string(iStep, '\t');

	std::osyncstream{ std::cout } << s_mapColors[eType] << (iStep ? (std::string(iStep, '\t') + strMessage) : strMessage) << s_mapColors[ETextTypeDefault] <<std::endl;
	std::osyncstream{ m_instance->m_file } << strMessage << std::endl;
}//Logger::LogImpl()


void Logger::Log(const std::string& strMessage, unsigned int iStep)
{
	LogImpl(strMessage, iStep, ETextTypeDefault);
}//Logger::Log()


void Logger::Success(const std::string& strMessage, unsigned int iStep)
{
	LogImpl(strMessage, iStep, ETextTypeSuccess);
}//Logger::Success()


void Logger::Warning(const std::string& strMessage, unsigned int iStep)
{
	LogImpl(strMessage, iStep, ETextTypeWarning);
}//Logger::Warning()


void Logger::Error(const std::string& strMessage, unsigned int iStep)
{
	LogImpl(strMessage, iStep, ETextTypeError);
}//Logger::Error()


void Logger::PrintProgress(const std::string& strPrefix, int iCount, int iMaxCount)
{
	std::osyncstream{ std::cout } << "\033[2K\r" << s_mapColors[ETextTypeSuccess] << std::format("{} : [{} / {}]", strPrefix, iCount, iMaxCount) << s_mapColors[ETextTypeDefault] << std::flush;
}//Logger::PrintProgress()