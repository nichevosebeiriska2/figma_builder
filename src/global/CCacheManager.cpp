#include <mutex>
#include <execution>

#include "CCacheManager.h"
#include "image_utils.h"
#include "json_utils.h"


rj::Document CCacheManager::FindElementJson(const std::string& strComponentName, ENodeType eType)
{
	auto path = GetElementsCacheDirectory() / ToString(eType) / (strComponentName + ".txt");
	if (std::filesystem::exists(path))
		return LoadJsonFromFile(path);

	return rj::Document(rj::kNullType);
}


std::unique_ptr<SStbImage> CCacheManager::FindElementImage(const std::string& strComponentName, ENodeType eType, float scale)
{
	std::string strSuffix = scale != 1.0f ? std::format("@{}x", scale) : "";
	auto path = GetImagesCacheDirectory() / ToString(eType) / (strComponentName + strSuffix + ".png");
	if (std::filesystem::exists(path))
		return LoadPngByStb(path.generic_string());

	return nullptr;
}


std::optional<std::filesystem::path> CCacheManager::FindElementImagePath(const std::string& strComponentName, ENodeType eType, float scale)
{
	std::string strSuffix = scale != 1.0f ? std::format("@{}x", scale) : "";
	auto path = GetImagesCacheDirectory() / ToString(eType) / (strComponentName + strSuffix + ".png");

	if (!std::filesystem::exists(path))
		return std::nullopt;

	return path;
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


void CCacheManager::SaveFigmaLibraryFile(rj::Value& figFile)
{
	SaveJsonToFileFast(figFile, GetCacheDirectory() / "library.txt");
	Logger::Log(std::format("CCacheManager::Init() - profile file saved to {}", GetCacheDirectory().generic_string()), 1);
}

void CCacheManager::SaveFigmaFileMeta(rj::Value& figFile)
{
	SaveJsonToFileFast(figFile, GetCacheDirectory() / "file_meta.txt");
	Logger::Log(std::format("CCacheManager::Init() - profile file saved to {}", GetCacheDirectory().generic_string()), 1);
}


void CCacheManager::SaveFigmaLibraryFileMeta(rj::Value& figFile)
{
	SaveJsonToFileFast(figFile, GetCacheDirectory() / "library_meta.txt");
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

rj::Document CCacheManager::TryGetProjectFile()
{
	auto doc = LoadJsonFromFileFast(GetCacheDirectory() / "figFile.txt");

	if (doc.HasParseError() || doc.IsNull())
		return rj::Document(rj::kNullType);

	return doc;
}


rj::Document CCacheManager::TryGetLibraryFile()
{
	auto doc = LoadJsonFromFileFast(GetCacheDirectory() / "library.txt");

	if (doc.HasParseError() || doc.IsNull())
		return rj::Document(rj::kNullType);

	return doc;
}


rj::Document CCacheManager::TryGetProjectInfo()
{
	auto doc = LoadJsonFromFile(GetCacheDirectory() / "file_meta.txt");

	if (doc.HasParseError() || doc.IsNull())
		return rj::Document(rj::kNullType);

	return doc;
}//CCacheManager::TryGetProjectInfo()


rj::Document CCacheManager::TryGetProjectLibraryInfo()
{
	auto doc = LoadJsonFromFile(GetCacheDirectory() / "library_meta.txt");

	if (doc.HasParseError() || doc.IsNull())
		return rj::Document(rj::kNullType);

	return doc;
}//CCacheManager::TryGetProjectLibraryInfo()


bool CCacheManager::IsProjectFileExists()
{
	return std::filesystem::exists(GetCacheDirectory() / "figFile.txt");
}//IsProjectFileExists()


bool CCacheManager::IsLibraryFileExists()
{
	return std::filesystem::exists(GetCacheDirectory() / "library.txt");
}//IsLibraryFileExists()()


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

		rj::Document docElementJson = CCacheManager::FindElementJson(strElementName, eNodeType);

		if (!docElementJson.IsNull())
		{
			Logger::Success(std::format("element {} and its image loaded from cache", strElementName));
			mapElements[strElementName] = std::move(docElementJson);
		}
	}

	return mapElements;
}


CCacheManager::TMapNodeTypeNode CCacheManager::LoadElementsFromCache()
{
	TMapNodeTypeNode mapResult;

	std::mutex mapMutex;

	auto lambda_load_from_dir = [&](auto& path)
	{
		const ENodeType eType = StringToNodeType(path.filename().generic_string());
		if (eType == ENodeType::_UNKNOWN)
		{
			Logger::Error(std::format("Cache directory contains dir with invalud name '{}'", path.filename().generic_string()), 2);
			return;
		}

		TMapStrJson mapElementJsonsFromDirectory = LoadElementsOfSameTypeFromCache(eType);
		std::lock_guard guard{ mapMutex };
		mapResult[eType] = std::move(mapElementJsonsFromDirectory);
	};

	std::vector<std::filesystem::path> vecDirectories;
	for (auto it : std::filesystem::directory_iterator(GetElementsCacheDirectory()))
		vecDirectories.emplace_back(it.path());

	std::for_each(std::execution::par, vecDirectories.begin(), vecDirectories.end(), lambda_load_from_dir);

	return mapResult;
}//CCacheManager::LoadElementsFromCache()
