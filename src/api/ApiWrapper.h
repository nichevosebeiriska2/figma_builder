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
	struct SImportParams
	{
		ENodeType eNodeType = ENodeType::_UNKNOWN;
		std::regex strRegExpr;
		ESliceFormat m_eSliceFormat = ESliceFormat_Ignore;
		ShadowBorder m_sBorder;
	};

	struct SElementToGather {
		ENodeType eType;
		std::string strName;
		IDs m_ids;
		const rj::Value* pJson;
		std::map<float, std::string> m_mapLinksToScaledImages;
	};

	using TVecImportParams = std::vector<SImportParams>;
	using TMapScaleSetIDs = std::map<float, std::set<IDs>>;
	using TMapSpecificScales = std::map<std::string, std::vector<float>>;
	using TMapShadowBorder = std::map<std::string, ShadowBorder>;
	using TMapDocuments = std::map<std::string, rj::Document>;

protected:
	static TVecImportParams s_vecParams;
	static TVecRegex s_vecRegexFilter;
	static TVecFloat s_vecDefaultImageScales;
	static TVecStr s_vecCanvases;
	static TVecStr s_vecLibraryCanvases;
	static TVecStr s_vecStyleCanvases;
	static TVecStr s_vecLibraryStyleCanvases;

	static TMapSpecificScales s_mapSpecificScales;
	static TMapShadowBorder s_mapBorders;
	static TMapDocuments m_mapElementsJsons;
	static rj::Document m_docProjectFile;
	static rj::Document m_docLibraryFile;

protected:

	// parsing
	static bool ParseTaskList(const rj::Value& arrayTasks);
	static bool ParseBorders(const rj::Value& objBorders);
	static bool ParseImportParams(const rj::Value& arrayParams, SImportParams& sParams);

	// check 
	static bool CheckLibraryFileNeedUpdate(rj::Document& docMetaFile);
	static bool CheckFileNeedUpdate(rj::Document& docMeta);

	// update
	static bool UpdateProjectLibraryFile();
	static bool UpdateProjectFile();

	static void GatherImages(std::vector<SElementToGather>& vecElements);
	static void GatherImageLinks(TMapScaleSetIDs& mapScaledIds, std::vector<SElementToGather>& vecElements); // get links to images of required scale
	static bool SetIgnoreFilters(TVecStr& vecFiltersIgnore);
	static SImportParams GuessImportParams(const ENodeType eType, const std::string& strElementName);

	static bool GatherGraphicalElements();
	static bool GatherStyles();

public:
	static bool Update();
	static bool Gather();
	static void BuildElementsFromCache();
	static bool InitConfig();
};