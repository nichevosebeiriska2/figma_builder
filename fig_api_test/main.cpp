
#include "ApiWrapper.h"
#include <iostream>
#include <filesystem>


bool OnCommandLineArguments(int argc, char** argv)
{
	if (!CResourceManager::InitConfig())
		return false;

	SetOutputDirectory(R"(E:/DOWNLOAD/_FIGMA/ygs)");

	TVecStr vecArguments;
	for (int i = 1; i < argc; i++)
		vecArguments.emplace_back(argv[i]);

	if (argc == 2 && std::string{ argv[1] } == "-delete_cache")
	{
		CCacheManager::DeleteCache();
		Logger::Success("Cache deleted", 2);
		return true;
	}
	if (argc == 2 && std::string{ argv[1] } == "-update")
		return CResourceManager::Update();

	else if (argc == 2 && std::string{ argv[1] } == "-gather")
		return CResourceManager::Gather();
	
	else if (argc == 2 && std::string{ argv[1] } == "-build_from_cache")
	{
		CResourceManager::BuildElementsFromCache();
		return true;
	}
	
	return false;
}


#include <windows.h>

std::filesystem::path GetExePath()
{
	wchar_t buffer[1024];
	GetModuleFileNameW(nullptr, buffer, 1024);
	return std::filesystem::path{ buffer };
}

int main(int argc, char** argv)
{
	Logger::InitInstance();
	CCacheManager::InitDirectories();

	SetOutputDirectory(R"(E:/DOWNLOAD/_FIGMA/ygs)");
	SetWorkingDirectory(GetExePath().parent_path());

	if (!OnCommandLineArguments(argc, argv))
	{
		Logger::Error("Invalid input");
		return -1;
	}

	return 0;
}