#pragma once 

#include "api_utils.h"
#include "json_utils.h"
#include "image_utils.h"

#include "global.h"

#include "CCacheManager.h"
#include <expected>
#include <regex>
#include <set>

class CResourceManager
{
public:
	struct SElementToGather {
		ENodeType eType;
		std::string strName;
		IDs m_ids;
		const rj::Value* pJson;
		std::map<float, std::string> m_mapLinksToScaledImages;
	};

protected:


	using TVecImportParams = std::vector<SImportParams>;

	static TVecImportParams s_vecParams;
	static TVecStr s_vecFiltersIgnore;
	static std::vector<std::regex> s_vecRegexFilter;
	static TVecFloat s_vecDefaultImageScales;
	static TMapVec<float> s_mapSpecificScales;
	static TVecStr s_vecCanvases;
	static std::map<std::string, ShadowBorder> s_mapBorders;
	static std::map<std::string, rj::Document> m_mapElementsJsons;
	static std::map<std::string, std::filesystem::path> m_mapElementImagePaths;

protected:
	static std::expected<rj::Document, std::string> GetFile();
	static std::vector<float> m_vecImageScales;

	static std::optional<rj::Document> GetActualFile(bool bForceUpdate = false);
	static void GatherImages(std::vector<SElementToGather>& vecElements);
	static bool IsNameIgnored(const std::string& strName);
	static void GatherImageLinks(std::map<float, std::set<IDs>>& mapScaledIds, std::vector<SElementToGather>& vecElements); // get links to images of required scale
	static std::optional<SImportParams> ParseImportParams(const rj::Value& arrayParams);

public:
	static std::optional<rj::Document> GetRawFile();
	static bool CheckFileNeedUpdate();
	static bool GatherGraphicalElements();
	//static std::expected<rj::Document, std::string> GetElement(const std::string& strElementName);
	static bool UpdateProjectFile();
	static void BuildElementsFromCache();
	static bool GetImageByIDs(IDs ids);
	static SImportParams GuessImportParams(const ENodeType eType, const std::string& strElementName);
	static bool ParseTaskList(const rj::Value& arrayTasks);
	static bool ParseBorders(const rj::Value& objBorders);

	static void SetCanvases(TVecStr& vecCanvases);
	static bool SetIgnoreFilters(TVecStr& vecFiltersIgnore);
	static void SetSpecificScales(TMapVec<float>& mapSpecificScales);
	static void SetDefaultScales(TVecFloat& vecDefaultScales);
};