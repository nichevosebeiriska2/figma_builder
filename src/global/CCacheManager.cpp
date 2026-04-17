#include "CCacheManager.h"
#include "image_utils.h"
#include "json_utils.h"


std::optional<rj::Document> CCacheManager::FindElementJson(const std::string& strComponentName, ENodeType eType)
{
	auto path = GetElementsCacheDirectory() / ToString(eType) / (strComponentName + ".txt");
	if (std::filesystem::exists(path))
		return LoadJsonFromFile(path);

	return std::nullopt;
}


std::optional<StbImage> CCacheManager::FindElementImage(const std::string& strComponentName, ENodeType eType, float scale)
{
	std::string strSuffix = scale != 1.0f ? std::format("@{}x", scale) : "";
	auto path = GetImagesCacheDirectory() / ToString(eType) / (strComponentName + strSuffix + ".png");
	if (std::filesystem::exists(path))
		return LoadPngByStb(path.generic_string());

	return std::nullopt;
}


void CCacheManager::SaveBuildElementJson(const rj::Value& jsonElem)
{
	auto type = jsonElem.HasMember("type") ? jsonElem["type"].GetString() : "unknown";

	SaveJsonToFile(jsonElem, (GetElementsCacheDirectory() / type / jsonElem["name"].GetString()).replace_extension("txt"));
	Logger::Log(std::format("CCacheManager::SaveElementJson() element '{}' saved to {}", jsonElem["name"].GetString(), GetCacheDirectory().generic_string()), 1);
}


void CCacheManager::SaveFigmaFile(rj::Value& figFile)
{
	SaveJsonToFileFast(figFile, GetCacheDirectory() / "figFile.txt");
	Logger::Log(std::format("CCacheManager::Init() - profile file saved to {}", GetCacheDirectory().generic_string()), 1);
}


void CCacheManager::DeleteCache()
{
	if (std::filesystem::exists(GetCacheDirectory()))
		std::filesystem::remove_all(GetCacheDirectory());
}


void CCacheManager::InitDirectories()
{
	if (!std::filesystem::exists(GetImagesCacheDirectory()))
		std::filesystem::create_directories(GetImagesCacheDirectory());

	if (!std::filesystem::exists(GetElementsCacheDirectory()))
		std::filesystem::create_directories(GetElementsCacheDirectory());
}

std::optional<rj::Document> CCacheManager::TryGetProjectFile()
{
	auto doc = LoadJsonFromFileFast(GetCacheDirectory() / "figFile.txt");

	if (doc.HasParseError() || doc.IsNull())
		return std::nullopt;

	return doc;
}

std::optional<rj::Document> CCacheManager::TryGetProjectInfo()
{
	auto doc = LoadJsonFromFile(GetCacheDirectory() / "file_meta.txt");

	if (doc.HasParseError() || doc.IsNull())
		return std::nullopt;

	return doc;
}

bool CCacheManager::IsProjectFileExists()
{
	return std::filesystem::exists(GetCacheDirectory() / "figFile.txt");
}


CCacheManager::TMapStrJson CCacheManager::LoadElementsOfSameTypeFromCache(ENodeType eType)
{
	TMapStrJson mapElements;
	auto pathToDir = GetElementsCacheDirectory() / ToString(eType);
	for (auto it : std::filesystem::recursive_directory_iterator(pathToDir))
	{
		if (!it.path().has_extension())
			continue;

		auto strElemType = it.path().parent_path().filename().string();
		auto eNodeType = StringToNodeType(strElemType);

		const std::string strElementName = it.path().filename().replace_extension().generic_string();

		if (strElementName.find("separator_s") != strElementName.npos)
			int a = 1;

		auto opt_json = CCacheManager::FindElementJson(strElementName, eNodeType);
		//auto opt_image = CCacheManager::FindElementImage(strElementName, eNodeType);

		if (opt_json)
		{
			Logger::Success(std::format("element {} and its image loaded from cache", strElementName));
			mapElements[strElementName] = std::move(*opt_json);
		}
		//else if (!opt_image)
			//Logger::Warning(std::format("failed to load image for element {}", strElementName));
	}

	return mapElements;

}

#include <mutex>
#include <execution>

CCacheManager::TMapNodeTypeNode CCacheManager::LoadElementsFromCache()
{
	TMapNodeTypeNode mapResult;

	std::mutex mapMutex;

	auto lambda_load_from_dir = [&](auto& path)
	{
			auto name = path.filename().generic_string();
			if (name.find("separator_s") != name.npos)
				int a = 1;
		const ENodeType eType = StringToNodeType(path.filename().generic_string());
		if (eType == ENodeType::_UNKNOWN)
		{
			Logger::Error(std::format("Cache directory contains dir with invalud name '{}'", path.filename().generic_string()), 2);
			return;
		}

		auto allFromDirectory = LoadElementsOfSameTypeFromCache(eType);
		std::lock_guard guard{ mapMutex };
		mapResult[eType] = std::move(allFromDirectory);
	};

	std::vector<std::filesystem::path> vecDirectories;
	for (auto it : std::filesystem::directory_iterator(GetElementsCacheDirectory()))
		vecDirectories.emplace_back(it.path());

	std::for_each(std::execution::par, vecDirectories.begin(), vecDirectories.end(), lambda_load_from_dir);

	return mapResult;
}//CCacheManager::LoadElementsFromCache()
