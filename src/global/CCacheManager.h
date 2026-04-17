#pragma once

#include <optional>

#include "image_utils.h"
#include "json_utils.h"
#include "CLogger.h"

class CCacheManager
{
public:
	using TMapStrJson = std::map<std::string, rj::Document>;
	using TMapNodeTypeNode = std::map<ENodeType, TMapStrJson>;

protected:
	static TMapStrJson LoadElementsOfSameTypeFromCache(ENodeType eType);

public:
	static rj::Document FindElementJson(const std::string& strComponentName, ENodeType eType);\
	static std::unique_ptr<SStbImage> FindElementImage(const std::string& strComponentName, ENodeType eType, float scale = 1.0f);
	static std::optional<std::filesystem::path> FindElementImagePath(const std::string& strComponentName, ENodeType eType, float scale = 1.0f);
	static rj::Document TryGetProjectFile();
	static rj::Document TryGetLibraryFile();
	static rj::Document TryGetProjectInfo();
	static rj::Document TryGetProjectLibraryInfo();

	static void SaveBuildElementJson(const rj::Value& jsonElem);
	static void SaveFigmaFile(rj::Value& figFile);
	static void SaveFigmaLibraryFile(rj::Value& figFile);
	static void SaveFigmaFileMeta(rj::Value& figFile);
	static void SaveFigmaLibraryFileMeta(rj::Value& figFile);
	static void DeleteCache();
	static void InitDirectories();

	static bool IsProjectFileExists();
	static bool IsLibraryFileExists();
	static TMapNodeTypeNode LoadElementsFromCache();
};