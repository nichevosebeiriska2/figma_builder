#include "ApiWrapper.h"

#include "import.h"
#include <regex>
#include <execution>
#include <algorithm>
#include <ranges>


std::map<std::string, rj::Document>				CResourceManager::m_mapElementsJsons{};
std::map<std::string, std::filesystem::path>	CResourceManager::m_mapElementImagePaths{};
std::vector<float>								CResourceManager::m_vecImageScales{};
std::vector<std::string>						CResourceManager::s_vecCanvases{};
std::map<std::string, ShadowBorder>				CResourceManager::s_mapBorders{};
CResourceManager::TVecImportParams				CResourceManager::s_vecParams{};
TVecStr											CResourceManager::s_vecFiltersIgnore{};
std::vector<std::regex>							CResourceManager::s_vecRegexFilter{};
TVecFloat										CResourceManager::s_vecDefaultImageScales{ 1.0f };
TMapVec<float>									CResourceManager::s_mapSpecificScales{};


std::expected<rj::Document, std::string> CResourceManager::GetFile()
{
	const SHttpResponseData response = CurlGetFile();

	if (response.status_code)
		return  std::unexpected{ response.body };

	rj::Document doc;
	doc.Parse(response.body.c_str(), response.body.size());

	if (doc.HasParseError())
		return std::unexpected{ "GetFile() : failed to parse json" };

	return std::move(doc);
}


bool CResourceManager::SetIgnoreFilters(TVecStr& vecFiltersIgnore)
{
	if (std::ranges::any_of(vecFiltersIgnore, [](const auto& expr) {return !IsRegExValid(expr);}))
		return false;

	s_vecFiltersIgnore = vecFiltersIgnore;

	try
	{
		for (const auto& str : s_vecFiltersIgnore)
			s_vecRegexFilter.emplace_back(std::regex(str));
	}
	catch (std::regex_error)
	{
		return false;
	}

	return true;
}


void CResourceManager::SetSpecificScales(TMapVec<float>& mapSpecificScales)
{
	s_mapSpecificScales = mapSpecificScales;
}


void CResourceManager::SetDefaultScales(TVecFloat& vecDefaultScales)
{
	s_vecDefaultImageScales = vecDefaultScales;
}


void CResourceManager::SetCanvases(TVecStr& vecCanvases)
{
	s_vecCanvases = vecCanvases;
}


std::optional<std::map<std::string, ShadowBorder>> ParseBordersImpl(const rj::Value& objBorders)
{
	if (!objBorders.IsObject())
		return std::nullopt;

	std::map<std::string, ShadowBorder> result;

	for (auto it = objBorders.MemberBegin(); it != objBorders.MemberEnd(); it++)
	{
		if (!it->value.IsArray())
			return std::nullopt;

		auto vecValues = ParseJsonArrayAsInts(it->value);
		
		if (!vecValues || vecValues->size() != 4)
			return std::nullopt;

		result[it->name.GetString()] = ShadowBorder{  (float)(*vecValues)[0]
													, (float)(*vecValues)[1]
													, (float)(*vecValues)[2]
													, (float)(*vecValues)[3] };
	}

	return result;
}


bool CResourceManager::ParseBorders(const rj::Value& objBorders)
{
	auto optBorders = ParseBordersImpl(objBorders);

	if (!optBorders)
		return false;

	s_mapBorders = *optBorders;

	return true;
}


std::optional<SImportParams> CResourceManager::ParseImportParams(const rj::Value& arrayParams)
{
	if (!arrayParams.IsArray() || arrayParams.Size() != 4)
	{
		Logger::Error("Invalid import params size");
		return std::nullopt;
	}

	auto optArrString = ParseJsonArrayAsStrings(arrayParams);

	if (!optArrString || (*optArrString).size() != 4)
		return std::nullopt;

	auto vecValues = *optArrString;
	auto fnPrintError = [&]()
	{
		Logger::Error(std::format("failed to parse task with params : '[{}], [{}], [{}], [{}]'", vecValues[0], vecValues[1], vecValues[2], vecValues[3]));
	};

	auto eType = StringToNodeType(vecValues[0]);

	if (eType == _UNKNOWN)
	{
		fnPrintError();
		return std::nullopt;
	}

	if (!s_mapBorders.contains(vecValues[3]))
	{
		fnPrintError();
		return std::nullopt;
	}

	auto eSliceFormat = StringToSliceFormat(vecValues[2]);

	if (eSliceFormat == ESliceFormat_Unknown)
	{
		fnPrintError();
		return std::nullopt;
	}

	if (!IsRegExValid(vecValues[1]))
		int a = 1;

	return SImportParams{ eType, std::regex{vecValues[1]}, eSliceFormat, s_mapBorders[vecValues[3]] };
}


bool CResourceManager::ParseTaskList(const rj::Value& arrayTasks)
{
	if (!arrayTasks.IsArray())
		return false;

	for (auto it = arrayTasks.Begin(); it != arrayTasks.End(); it++)
	{
		if (!it->IsArray())
			return false;

		auto optTask = ParseImportParams(*it);

		if (!optTask)
		{
			Logger::Error("Failed to parse task list");
			return false;
		}

		s_vecParams.emplace_back(*optTask);
	}

	return true;
}


std::optional<rj::Document> CResourceManager::GetRawFile()
{
	if (auto exp_file = GetFile())
	{
		CCacheManager::SaveFigmaFile(*exp_file);
		return std::move(*exp_file);
	}

	return std::nullopt;
}


std::optional<rj::Document> CResourceManager::GetActualFile(bool bForceUpdate)
{
	const bool bNeedUpdate = CheckFileNeedUpdate() || bForceUpdate;
	auto opt_file = 
		//bNeedUpdate ? CResourceManager::GetRawFile() : 
		CCacheManager::TryGetProjectFile();

	if (!bNeedUpdate && !opt_file)
		opt_file = CResourceManager::GetRawFile();

	if (!opt_file)
		return std::nullopt;

	return opt_file;
}


bool CResourceManager::CheckFileNeedUpdate()
{
	auto opt_cached_project_meta = CCacheManager::TryGetProjectInfo();

	if (!opt_cached_project_meta)
	{
		Logger::Log("Failed to find file meta-info. Updating...");

		auto r = CurlGetFileInfo();

		rj::Document doc(rj::kObjectType);
		doc.Parse(r.body.c_str());

		SaveJsonToFile(doc, GetCacheDirectory() / "file_meta.txt");

		return true;
	}

	Logger::Log("Checking if project need to be updated...");
	auto r = CurlGetFileInfo();

	rj::Document doc(rj::kObjectType);
	doc.Parse(r.body.c_str());

	auto& meta = *opt_cached_project_meta;

	const std::string strCachedVersion = meta["version"].GetString();
	const std::string strApiFileVersion = doc["version"].GetString();

	if (strCachedVersion != strApiFileVersion)
	{
		Logger::Log(std::format("Different version - cached : {}, actual : {}", strCachedVersion, strApiFileVersion));
		SaveJsonToFile(doc, GetCacheDirectory() / "file_meta.txt");
		return true;
	}

	if (!CCacheManager::IsProjectFileExists())
	{
		Logger::Log("Failed to load cached project file. Updating...");
		return true;
	}

	Logger::Log(std::format("project name : {}\nlast modified : {}\nversion : {}", doc["name"].GetString(), doc["lastModified"].GetString(), doc["version"].GetString()));
	Logger::Log("No update needed. Running from cache");

	return false;
}


bool CResourceManager::GetImageByIDs(IDs ids)
{
	auto resp = CurlGetImage({ ids }, 1.0f, true);

	rj::Document docResp;
	docResp.Parse(resp.body.c_str());

	if (docResp.HasParseError())
	{
		Logger::Error(std::format("GatherImages() : CurlGetImage() returned invalid responce : {}", resp.body));
		return false;
	}

	const auto& doc_images = docResp["images"].GetObject();
	std::vector<DownloadTask> tasks;
	const std::string strImageName = "image_requested";

	for (auto it_image = doc_images.MemberBegin(); it_image != doc_images.MemberEnd(); it_image++)
	{
		auto req_ids = GetElementIDs(it_image->name.GetString());
		const std::string& img_name = strImageName;

		if (it_image->value.IsString())
			tasks.emplace_back(it_image->value.GetString(), (GetImagesCacheDirectory() / (img_name + ".png")).generic_string());
		else
		{
			Logger::Warning(std::format("failed to load image for element {} with ids '{}:{}'", img_name, req_ids.first, req_ids.second));
			return false;
		}

		//}
	}

	CurlDownloadImagesAsync(tasks);

	return true;
}


bool CResourceManager::IsNameIgnored(const std::string& strName)
{
	return std::ranges::any_of(s_vecRegexFilter, [&](const auto& regex) { return std::regex_match(strName, regex);});
}


void CResourceManager::GatherImageLinks(std::map<float, std::set<IDs>>& mapScaledIds, std::vector<SElementToGather>& vecElements)
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
}


void CResourceManager::GatherImages(std::vector<SElementToGather>& vecElements)
{
	// gathering all required scales for each element
	std::map<float, std::set<IDs>> mapElementsWithScale;

	auto lambda_get_ids = [](auto& elem) {return elem.m_ids;};

	// insert images with default scales
	for (const auto&scale : s_vecDefaultImageScales)
		mapElementsWithScale[scale].insert_range(vecElements | std::views::transform(lambda_get_ids) | std::ranges::to<std::set>());

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
	std::vector<DownloadTask> tasks;
	for (const auto& element : vecElements)
	{
		if (element.strName.find("button_fold") != element.strName.npos)
			int a = 1;

		for (auto& [scale, url] : element.m_mapLinksToScaledImages)
		{
			const std::string strOutputImageName = element.strName + lambda_create_scale_suffix(scale) + ".png";
			tasks.emplace_back(url, (GetImagesCacheDirectory() / ToString(element.eType) / (strOutputImageName)).generic_string());
		}
	}

	Logger::Success(std::format("downloading '{}' images ...", tasks.size()));

	// downloading scaled images ...
	CurlDownloadImagesAsync(tasks);

	return;
}


bool CResourceManager::GatherGraphicalElements()
{
	if (CheckFileNeedUpdate())
		Logger::Warning("File is out of date. Run with '-update' if you want to update it");
	
	auto opt_file = CCacheManager::TryGetProjectFile();

	if (!opt_file)
	{
		Logger::Error("Failed to get file from cache. No file found. Try run utility with '-update'");
		return false;
	}

	Logger::Success("Project file parsed", 1);
		
	auto& file = *opt_file;

	std::vector<rj::Value*> result;
	

	std::vector<const rj::Value*> graphical_elements_from_components;
	std::vector<rj::Value*> vecFrom;

	for(auto& strCanvas : s_vecCanvases)
		findNodesByNameAndType(file, strCanvas, CANVAS, vecFrom);
	
	if (vecFrom.empty())
	{
		Logger::Error("No canvases to find elements in. Error");
		return false;
	}
	for(auto& canvas : vecFrom)
		findElementNodesFromExportDev(*canvas, graphical_elements_from_components);


	if (graphical_elements_from_components.empty())
	{
		Logger::Error("Failed to find graphical elements. Error", 2);
		return false;
	}

	std::vector<SElementToGather> vecElementsToGather;

	for (auto* elem : graphical_elements_from_components)
	{
		if (IsNameIgnored((*elem)["name"].GetString()))
			continue;
		//elem
		CCacheManager::SaveBuildElementJson(*elem);
		auto type = StringToNodeType((*elem)["type"].GetString());

		vecElementsToGather.emplace_back(type, (*elem)["name"].GetString(), GetElementIDs((*elem)["id"].GetString()), elem );
	}

	std::map<std::string, IDs> m_mapNameIds; // for gather images;

	GatherImages(vecElementsToGather);

	return true;
}


bool CResourceManager::UpdateProjectFile()
{
	if (!CheckFileNeedUpdate())
	{
		Logger::Success("File is up to date. No update needed");
		return true;
	}
	else
	{
		Logger::Log("Trying to get file with figma API ...", 1);
		auto opt_file = GetRawFile();

		if (!opt_file)
		{
			Logger::Error("Failed to get file from API. Error.", 2);
			return false;
		}

		Logger::Success("Figma project file successfully saved to cache.");
		return true;
	}


	return false;
}


SImportParams CResourceManager::GuessImportParams(const ENodeType eType,const std::string& strElementName)
{
	try
	{

		for (const auto& [eNodeType, str_regexp, fmt, fmt_border] : s_vecParams)
		{
			//auto regexp = std::regex(str_regexp);
			try
			{
				if (eType == eNodeType && std::regex_search(strElementName, str_regexp))
					return { eNodeType, str_regexp, fmt, fmt_border };
			}
			catch (...)
			{
				int a = 1;
			}
		}
	}
	catch (...)
	{
		int a = 1;
	}

	return SImportParams{};
}



void CResourceManager::BuildElementsFromCache()
{
	CCacheManager::TMapNodeTypeNode mapElement = CCacheManager::LoadElementsFromCache();

	UINT iBuildedElements	= 0;
	UINT iErrorElements		= 0;
	UINT iIgnoredElements	= 0;

	auto lambda_build_elem_with_scale = [&](const std::string& name, rj::Value& json, float scale) {

		if (name.find("grid_header_left") != name.npos)
			int a = 1;

			const ENodeType eType = StringToNodeType(json["type"].GetString());
			auto opt_image = CCacheManager::FindElementImage(name, eType, scale);

			
			if (!opt_image)
			{
				iErrorElements++;
				//Logger::Warning(std::format("failed to load image for element [name : [{}], type : [{}], scale : [{}]]", name, ToString(eType), scale));
				
			}
			else
			{
				auto [type, strRegExp, slice_fmt, border_fmt] = GuessImportParams(eType, name);

				if (slice_fmt != ESliceFormat_Ignore && type != _UNKNOWN)
				{
					if (CImporter::Import(name, json, *opt_image, slice_fmt, border_fmt, scale)) {
						iBuildedElements++;
						//Logger::Success(std::format("for element [name : [{}], type : [{}], scale : [{}]] import format guessed", name, ToString(eType), scale));
					}

					else
					{
						iErrorElements++;
						//Logger::Error(std::format("failed to import element [name : [{}], type : [{}], scale : [{}]", name, ToString(eType), scale));
					}
				}
				else
					iIgnoredElements++;
			}
		};

	auto lambda_build_elem = [&](auto& elem) {

		auto& [name, json] = elem;
		auto lambda_filter = [&](const std::string& strElementName, const std::string& strFilter) {
				return std::regex_match(strElementName, std::regex(strFilter));
			};

		for (auto& [filter, vecScales] : s_mapSpecificScales)
		{
			if (lambda_filter(name, filter))
				for(auto& scale : vecScales)
					lambda_build_elem_with_scale(name, json, scale);

		}

		for(auto& scale : s_vecDefaultImageScales)
			lambda_build_elem_with_scale(name, json, scale);
		
		};

	for (auto& [node_type, mapElementsOfthisType] : mapElement)
		std::for_each(std::execution::par, std::begin(mapElementsOfthisType), std::end(mapElementsOfthisType), lambda_build_elem);

	Logger::Log(
	std::format(
		R"(
	Statistics : 
		Builded : {}
		Ignored : {}
		Failed : {}
		)", iBuildedElements, iIgnoredElements, iErrorElements)
		, 1
	);
}