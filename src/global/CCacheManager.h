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
	static std::optional<rj::Document> FindElementJson(const std::string& strComponentName, ENodeType eType);
	static std::optional<StbImage> FindElementImage(const std::string& strComponentName, ENodeType eType, float scale = 1.0f);
	static std::optional<rj::Document> TryGetProjectFile();
	static std::optional<rj::Document> TryGetProjectInfo();

	static void SaveBuildElementJson(const rj::Value& jsonElem);
	static void SaveFigmaFile(rj::Value& figFile);
	static void DeleteCache();
	static void InitDirectories();

	static bool IsProjectFileExists();
	static TMapNodeTypeNode LoadElementsFromCache();
};