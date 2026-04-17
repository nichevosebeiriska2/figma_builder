#include "ApiWrapper.h"

#include "import.h"
#include <regex>
#include <execution>
#include <algorithm>
#include <error/en.h>
#include <ranges>
#include <format>

rj::Document									CResourceManager::m_docProjectFile;
rj::Document									CResourceManager::m_docLibraryFile;

TVecStr											CResourceManager::s_vecCanvases{};
TVecStr											CResourceManager::s_vecLibraryCanvases{};
TVecStr											CResourceManager::s_vecStyleCanvases{};
TVecStr											CResourceManager::s_vecLibraryStyleCanvases{};
TVecRegex										CResourceManager::s_vecRegexFilter{};
TVecFloat										CResourceManager::s_vecDefaultImageScales{ 1.0f };
CResourceManager::TVecImportParams				CResourceManager::s_vecParams{};

CResourceManager::TMapShadowBorder				CResourceManager::s_mapBorders{};
CResourceManager::TMapDocuments					CResourceManager::m_mapElementsJsons{};
CResourceManager::TMapSpecificScales			CResourceManager::s_mapSpecificScales{};


bool CResourceManager::SetIgnoreFilters(TVecStr& vecFiltersIgnore)
{
	bool bSuccess = true;

	for (const auto& str : vecFiltersIgnore)
	{
		s_vecRegexFilter.emplace_back(TryCreateRegex(str, bSuccess));
		if (!bSuccess)
			return false;
	}
	
	return true;
}//CResourceManager::SetIgnoreFilters()


bool CResourceManager::InitConfig()
{
	const std::filesystem::path pathToConfig = GetConfigFilePath();

	if (!std::filesystem::exists(pathToConfig))
	{
		Logger::Log("failed to find info file. Please set info");
		return false;
	}

	rj::Document docConfig = LoadJsonFromFile(pathToConfig);

	if (docConfig.HasParseError())
	{
		rj::GetParseError_En(docConfig.GetParseError());
		Logger::Error(std::format("Config '{}' has parse error \n\t error : '{}'\n\t offset '{}'"
			, pathToConfig.generic_string()
			, rj::GetParseError_En(docConfig.GetParseError())
			, docConfig.GetErrorOffset()));

		return false;
	}

	if (!docConfig.HasMember("token") || !docConfig["token"].IsString())
	{
		Logger::Error("LoadAccessParameters() - no 'token' field or field is not a string");
		return false;
	}

	if (!docConfig.HasMember("file descriptor") || !docConfig["file descriptor"].IsString())
	{
		Logger::Error("LoadAccessParameters() - no 'file descriptor' field or field is not a string");
		return false;
	}

	if (!docConfig.HasMember("library descriptor") || !docConfig["library descriptor"].IsString())
	{
		Logger::Error("LoadAccessParameters() - no 'library descriptor' field or field is not a string");
		return false;
	}

	SetAccessToken(docConfig.HasMember("token") ? docConfig["token"].GetString() : "");
	SetFileDescriptor(docConfig.HasMember("file descriptor") ? docConfig["file descriptor"].GetString() : "");
	SetFileLibraryDescriptor(docConfig.HasMember("library descriptor") ? docConfig["library descriptor"].GetString() : "");

	if (!docConfig.HasMember("import"))
	{
		Logger::Error("failed to load import settings from config");
		return false;
	}

	const auto& objImport = docConfig["import"];
	if (!objImport.HasMember("canvases"))
	{
		Logger::Error("failed to load 'canvases' array from config");
		return false;
	}

	if (!objImport.HasMember("style canvases"))
	{
		Logger::Error("failed to load 'canvases' array from config");
		return false;
	}

	if (!objImport.HasMember("library canvases"))
	{
		Logger::Error("failed to load 'canvases' array from config");
		return false;
	}

	if (!objImport.HasMember("library style canvases"))
	{
		Logger::Error("failed to load 'library canvases' array from config");
		return false;
	}

	if (!objImport.HasMember("image scales") || !objImport["image scales"].IsArray())
	{
		Logger::Error("Error : 'import' has no 'image scales' array");
		return false;
	}

	if (!objImport.HasMember("borders"))
	{
		Logger::Error("No borderd in config");
		return false;
	}

	if (!objImport.HasMember("task list"))
	{
		Logger::Error("No tasks in config");
		return false;
	}


	if (objImport.HasMember("ignore"))
	{
		TVecStr vecStrFilters;
		
		if (!ParseJsonArrayAsStrings(objImport["ignore"], vecStrFilters))
		{
			Logger::Error("'ignore' - all filter must be string type", 2);
			return false;
		}

		if (!CResourceManager::SetIgnoreFilters(vecStrFilters))
		{
			Logger::Error("invalid regular expression inside 'ignore'");
			return false;
		}
	}

	if (!ParseJsonArrayAsStrings(objImport["canvases"], s_vecCanvases))
	{
		Logger::Error("'canvases' - all elements must be string type", 2);
		return false;
	}

	if (!ParseJsonArrayAsStrings(objImport["style canvases"], s_vecStyleCanvases))
	{
		Logger::Error("'style canvases' - all elements must be string type", 2);
		return false;
	}

	if (!ParseJsonArrayAsStrings(objImport["library style canvases"], s_vecLibraryCanvases))
	{
		Logger::Error("'library canvases' - all elements must be string type", 2);
		return false;
	}

	if (!ParseJsonArrayAsFloats(objImport["image scales"], s_vecDefaultImageScales))
	{
		Logger::Error("all scales have to be float type'");
		return false;
	}

	if (objImport.HasMember("specific scales"))
	{
		const auto& arrSpecScales = objImport["specific scales"];
		for (auto it = arrSpecScales.MemberBegin(); it != arrSpecScales.MemberEnd(); it++)
			if (!ParseJsonArrayAsFloats(it->value, s_mapSpecificScales[it->name.GetString()]))
			{
				Logger::Error(std::format("specific scales for element '{}' - all scales have to be float type", it->name.GetString()), 2);
				return false;
			}
	}


	if (!CResourceManager::ParseBorders(objImport["borders"]))
	{
		Logger::Error("Failed to parser 'borders' list");
		return false;
	}

	if (!CResourceManager::ParseTaskList(objImport["task list"]))
	{
		Logger::Error("Failed to parser 'task list'");
		return false;
	}


	return true;
}


bool CResourceManager::ParseBorders(const rj::Value& objBorders)
{
	if (!objBorders.IsObject())
		return false;

	for (auto it = objBorders.MemberBegin(); it != objBorders.MemberEnd(); it++)
	{
		if (!it->value.IsArray())
			return false;

		TVecInts vecInts;

		if (!ParseJsonArrayAsInts(it->value, vecInts) || vecInts.size() != 4)
			return false;

		s_mapBorders[it->name.GetString()] = ShadowBorder{ (float)(vecInts)[0], (float)(vecInts)[1], (float)(vecInts)[2], (float)(vecInts)[3] };
	}

	return true;
}//CResourceManager::ParseBorders()


bool CResourceManager::ParseImportParams(const rj::Value& arrayParams, SImportParams& sParams)
{
	TVecStr vecStrTaskParams;
	if (!arrayParams.IsArray() || !ParseJsonArrayAsStrings(arrayParams, vecStrTaskParams) || vecStrTaskParams.size() != 4)
	{
		Logger::Error("Invalid import params size");
		return false;
	}

	const ENodeType eType = StringToNodeType(vecStrTaskParams[0]);
	const ESliceFormat eSliceFormat = StringToSliceFormat(vecStrTaskParams[2]);

	if (eType == _UNKNOWN || !s_mapBorders.contains(vecStrTaskParams[3]) || eSliceFormat == ESliceFormat_Unknown)
	{
		Logger::Error(std::format("failed to parse task with params : '[{}], [{}], [{}], [{}]'", vecStrTaskParams[0], vecStrTaskParams[1], vecStrTaskParams[2], vecStrTaskParams[3]));
		return false;
	}

	bool bSuccess = true;
	sParams = SImportParams{ eType, TryCreateRegex(vecStrTaskParams[1], bSuccess), eSliceFormat, s_mapBorders[vecStrTaskParams[3]] };

	if (!bSuccess)
	{
		Logger::Error(std::format("Invalid regex : {}", vecStrTaskParams[1]));
		return false;
	}

	return true;
}//CResourceManager::ParseImportParams()


bool CResourceManager::ParseTaskList(const rj::Value& arrayTasks)
{
	if (!arrayTasks.IsArray())
		return false;

	for (auto it = arrayTasks.Begin(); it != arrayTasks.End(); it++)
		if (!it->IsArray() || !ParseImportParams(*it, s_vecParams.emplace_back()))
		{
			Logger::Error("Failed to parse task list");
			return false;
		}

	return true;
}//CResourceManager::ParseTaskList()


bool IsDifferentVersions(const rj::Document& docMetaFromApi, const rj::Document& docCurrentMeta)
{
	const std::string strCachedVersion = docCurrentMeta["version"].GetString();
	const std::string strApiFileVersion = docMetaFromApi["version"].GetString();

	if(strCachedVersion != strApiFileVersion)
		Logger::Log(std::format("Different version - cached : {}, actual : {}", strCachedVersion, strApiFileVersion));
	
	return strCachedVersion != strApiFileVersion;
}//IsDifferentVersions()


bool CResourceManager::CheckLibraryFileNeedUpdate(rj::Document& docMeta)
{
	rj::Document docMetaCurrent = CCacheManager::TryGetProjectLibraryInfo();

	docMeta = CurlGetFileInfoAsJsonDocument();

	if (docMetaCurrent.IsNull())
	{
		Logger::Log("Failed to find file meta-info. Updating...");
		return true;
	}

	if (IsDifferentVersions(docMeta, docMetaCurrent))
		return true;

	if (!CCacheManager::IsProjectFileExists())
	{
		Logger::Log("Failed to load cached project file. Updating...");
		return true;
	}

	Logger::Log(std::format("project name : {}\nlast modified : {}\nversion : {}", docMetaCurrent["name"].GetString(), docMetaCurrent["lastModified"].GetString(), docMetaCurrent["version"].GetString()));
	Logger::Log("No update needed. Running from cache");

	return false;
}//CResourceManager::CheckLibraryFileNeedUpdate()



bool CResourceManager::CheckFileNeedUpdate(rj::Document& docMeta)
{
	rj::Document docMetaCurrent = CCacheManager::TryGetProjectInfo();

	docMeta = CurlGetFileInfoAsJsonDocument();

	if (docMetaCurrent.IsNull())
	{
		Logger::Log("Failed to find file meta-info. Updating...");
		return true;
	}

	if (IsDifferentVersions(docMeta, docMetaCurrent))
		return true;

	if (!CCacheManager::IsProjectFileExists())
	{
		Logger::Log("Failed to load cached project file. Updating...");
		return true;
	}

	Logger::Log(std::format("project name : {}\nlast modified : {}\nversion : {}", docMetaCurrent["name"].GetString(), docMetaCurrent["lastModified"].GetString(), docMetaCurrent["version"].GetString()));
	Logger::Log("No update needed. Running from cache");

	return false;
}//CResourceManager::CheckFileNeedUpdate()


void CResourceManager::GatherImageLinks(TMapScaleSetIDs& mapScaledIds, std::vector<SElementToGather>& vecElements)
{
	std::vector<SHttpResponseData> responses;
	std::map<float, std::vector<SHttpResponseData>> mapResponses;

	auto map = RequestImageLinksAsync(mapScaledIds, mapResponses);

	for (auto& [scale, vecDocuments] : map)
	{
		for (auto& docResponse : vecDocuments)
		{
			auto& images = docResponse["images"];
			for (auto it = images.MemberBegin(); it != images.MemberEnd();it++) {
				if (it->value.IsNull() || !it->value.IsString())
				{
					Logger::Warning(std::format("For element with ids '{}' api returned 'null'. Element has no bitmap due to internal server render error or invalid element ids", it->name.GetString()));
					continue;
				}
				IDs ids = GetElementIDs(it->name.GetString());
				std::string strImageUrl = it->value.GetString();
				auto it_elem = std::ranges::find_if(vecElements, [&](auto el) {return el.m_ids == ids;});
				
				if (it_elem != vecElements.end())
					it_elem->m_mapLinksToScaledImages[scale] = strImageUrl;
				
			}
		}
	}
}//CResourceManager::GatherImageLinks()


void CResourceManager::GatherImages(std::vector<SElementToGather>& vecElements)
{
	// gathering all required scales for each element
	std::map<float, std::set<IDs>> mapElementsWithScale;

	// insert images with default scales
	for (const auto&scale : s_vecDefaultImageScales)
		mapElementsWithScale[scale].insert_range(vecElements | std::views::transform([](auto& elem) {return elem.m_ids;}) | std::ranges::to<std::set>());

	// insert images with specific scales from config
	for (const auto& [strFilter, vecScales] : s_mapSpecificScales)
	{
		std::vector<SElementToGather> vecSpecificElements;
		TVecIDs ids;
		auto lambda_filter = [&](const SElementToGather& element) {return std::regex_match(element.strName, std::regex(strFilter));};

		auto setElementsWithSpecificScale = vecElements 
											| std::views::filter(lambda_filter) 
											| std::views::transform([](auto& elem) {return elem.m_ids;})
											| std::ranges::to<std::set>();
		
		for (const auto& scale : vecScales)
			mapElementsWithScale[scale].insert_range(setElementsWithSpecificScale);
	}

	// for each element gather links to its scaled images
	GatherImageLinks(mapElementsWithScale, vecElements);

	// now gather images

	auto lambda_create_scale_suffix = [](float scale) {return scale != 1.0f ? std::format("@{}x", scale) : "";};

	std::vector<SDownloadTask> tasks;
	for (const auto& element : vecElements)
		for (auto& [scale, url] : element.m_mapLinksToScaledImages)
		{
			const std::string strOutputImageName = element.strName + lambda_create_scale_suffix(scale) + ".png";
			tasks.emplace_back(url, (GetImagesCacheDirectory() / ToString(element.eType) / (strOutputImageName)).generic_string());
		}

	Logger::Success(std::format("downloading '{}' images ...", tasks.size()));

	// downloading scaled images ...
	CurlDownloadImagesAsync(tasks);

	return;
}//CResourceManager::GatherImages()


bool CResourceManager::GatherStyles()
{	
	TVecValuePtrs vecFrom;

	FindNodesByNameAndType(m_docProjectFile, s_vecStyleCanvases, CANVAS, vecFrom);
	FindNodesByNameAndType(m_docLibraryFile, s_vecLibraryStyleCanvases, CANVAS, vecFrom);

	TVecValuePtrs textGuideFrames;

	for (const auto& canvas : vecFrom)
		FindNodesByNameAndType(*canvas, TVecStr{ "text_guide",  "text guide" }, FRAME, textGuideFrames);

	TVecValuePtrs vecTextNodePtrs;

	for (const TValue* pValueTextGuideFrame : textGuideFrames)
		findNodesByType(*pValueTextGuideFrame, TEXT, vecTextNodePtrs);

	rj::Document docStyles(rj::kArrayType);

	for (const TValue* pTextNode : vecTextNodePtrs)
		docStyles.PushBack(std::move(*const_cast<TValue*>(pTextNode)), docStyles.GetAllocator());
	
	SaveJsonToFile(docStyles, GetOutputDirectory() / "styles.txt");
	
	return true;
}//CResourceManager::GatherStyles()


bool CResourceManager::GatherGraphicalElements()
{
	Logger::Success("Project file parsed", 1);

	TVecValuePtrs vecValuePtrFromComponents;
	TVecValuePtrs vecValueCanvases;

	FindNodesByNameAndType(m_docProjectFile, s_vecCanvases, CANVAS, vecValueCanvases);
	
	if (vecValueCanvases.empty())
	{
		Logger::Error("No canvases to find elements in. Error");
		return false;
	}

	for(const TValue* pValueCanvas : vecValueCanvases)
		FindElementsFromComponents(*pValueCanvas, vecValuePtrFromComponents);


	if (vecValuePtrFromComponents.empty())
	{
		Logger::Error("Failed to find graphical elements. Error", 2);
		return false;
	}

	std::vector<SElementToGather> vecElementsToGather;

	for (const TValue* pValue : vecValuePtrFromComponents)
	{
		const bool bNameIgnored = std::ranges::any_of(s_vecRegexFilter, [&](const auto& regex) { return std::regex_match((*pValue)["name"].GetString(), regex);});
		if (bNameIgnored)
			continue;
		
		CCacheManager::SaveBuildElementJson(*pValue);
		const std::string strElementName = (*pValue)["type"].GetString();
		vecElementsToGather.emplace_back(StringToNodeType(strElementName), strElementName, GetElementIDs((*pValue)["id"].GetString()), pValue);
	}

	GatherImages(vecElementsToGather);

	return true;
}


bool CResourceManager::UpdateProjectLibraryFile()
{
	rj::Document docMetaFileFromApi;
	bool bFileNeedUpdate = CheckLibraryFileNeedUpdate(docMetaFileFromApi);
	bool bLibFileExists = CCacheManager::IsLibraryFileExists();

	if (!bFileNeedUpdate && bLibFileExists)
	{
		// nothing to save
		Logger::Success("Library file is up to date. No update needed");
		return true;
	}

	if (!bLibFileExists || bFileNeedUpdate) // if there is no library file - download it
	{
		Logger::Log("Trying to get library file with figma API ...", 1);
		rj::Document docLibraryFile = CurlGetLibraryFile();

		if (docLibraryFile.IsNull())
		{
			Logger::Error("Failed to get file from API. Error.", 2);
			return false;
		}

		CCacheManager::SaveFigmaLibraryFile(docLibraryFile);
		Logger::Success("Figma library project file successfully saved to cache.");
	}

	// library file successfully updated yet. save lib meta file
	CCacheManager::SaveFigmaLibraryFileMeta(docMetaFileFromApi);
	return true;
}


bool CResourceManager::UpdateProjectFile()
{
	rj::Document docMetaFileFromApi;
	bool bFileNeedUpdate = CheckFileNeedUpdate(docMetaFileFromApi);
	bool bLibFileExists = CCacheManager::IsProjectFileExists();

	if (!bFileNeedUpdate && bLibFileExists)
	{
		// nothing to save
		Logger::Success("Project file is up to date. No update needed");
		return true;
	}

	if (!bLibFileExists || bFileNeedUpdate) // if there is no library file - download it
	{
		Logger::Log("Trying to get file with figma API ...", 1);
		rj::Document docFile = CurlGetFile();

		if (docFile.IsNull())
		{
			Logger::Error("Failed to get file from API. Error.", 2);
			return false;
		}

		CCacheManager::SaveFigmaFile(docFile);
		Logger::Success("Figma project file successfully saved to cache.");
	}

	// project file successfully updated yet. save lib meta file
	CCacheManager::SaveFigmaFileMeta(docMetaFileFromApi);
	return true;
}


bool CResourceManager::Update()
{
	return UpdateProjectLibraryFile() && UpdateProjectFile();
}


bool CResourceManager::Gather()
{
	if (!CResourceManager::Update())
	{
		Logger::Error("Failed to update project files");
		return false;
	}

	m_docLibraryFile = CCacheManager::TryGetLibraryFile();
	m_docProjectFile = CCacheManager::TryGetProjectFile();

	if (m_docProjectFile.IsNull() || m_docLibraryFile.IsNull())
	{
		Logger::Error("Failed to initialize project/library file");
		return false;
	}

	return GatherStyles() && GatherGraphicalElements();
}//CResourceManager::Gather()


CResourceManager::SImportParams CResourceManager::GuessImportParams(const ENodeType eType,const std::string& strElementName)
{
	for (const auto& [eNodeType, strRegexp, fmt, fmt_border] : s_vecParams)
		if (eType == eNodeType && std::regex_search(strElementName, strRegexp))
			return { eNodeType, strRegexp, fmt, fmt_border };

	return SImportParams{};
}//CResourceManager::GuessImportParams()


void CResourceManager::BuildElementsFromCache()
{
	CCacheManager::TMapNodeTypeNode mapElement = CCacheManager::LoadElementsFromCache();

	UINT iBuildedElements	= 0;
	UINT iErrorElements		= 0;
	UINT iIgnoredElements	= 0;

	auto lambdaBuildElemWithScale = [&](const std::string& name, rj::Value& json, float scale) {

			const ENodeType eType = StringToNodeType(json["type"].GetString());
			TPtrImage pImage = CCacheManager::FindElementImage(name, eType, scale);

			if (!pImage)
				iErrorElements++;
			else
			{
				SImportParams sParams = GuessImportParams(eType, name);

				CImporter::Import(name, json, *pImage, sParams.m_eSliceFormat, sParams.m_sBorder, scale);
			}
		};

	auto lambdaBuildElem = [&](auto& pairNameJson) {
		auto& [strElementName, docElementJson] = pairNameJson;

		auto lambda_filter = [&](const std::string& strElementName, const std::string& strFilter) {
			return std::regex_match(strElementName, std::regex(strFilter));
		};

		// build with specific scale
		for (auto& [strFilter, vecScales] : s_mapSpecificScales)
			if (lambda_filter(strElementName, strFilter))
				for(auto& fScale : vecScales)
					lambdaBuildElemWithScale(strElementName, docElementJson, fScale);

		//build with default scale
		for(const float fScale : s_vecDefaultImageScales)
			lambdaBuildElemWithScale(strElementName, docElementJson, fScale);
		
	};

	for (auto& [eNodeType, mapElementsOfthisType] : mapElement)
		std::for_each(std::execution::par, std::begin(mapElementsOfthisType), std::end(mapElementsOfthisType), lambdaBuildElem);
}