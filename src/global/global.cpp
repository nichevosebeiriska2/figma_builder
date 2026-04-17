
#include <regex>

#include "global.h"
#include "document.h"

static std::filesystem::path g_pathWorkingDirectory = std::filesystem::current_path();
static std::filesystem::path g_pathOutputDirectory = std::filesystem::current_path();

static std::string g_strAccessToken = "";
static std::string g_strFileDescriptor = "";
static std::string g_strCloneName = "";


std::string ToString(ENodeType eType)
{
	static std::map<ENodeType, std::string> s_mapTypeNamesTypeName
	{
		  {DOCUMENT, "DOCUMENT"}
		, {CANVAS, "CANVAS"}
		, {FRAME, "FRAME"}
		, {_GROUP, "GROUP"}
		, {TRANSFORM_GROUP, "TRANSFORM_GROUP"}
		, {SECTION, "SECTION"}
		, {VECTOR, "VECTOR"}
		, {BOOLEAN_OPERATION, "BOOLEAN_OPERATION"}
		, {STAR, "STAR"}
		, {LINE, "LINE"}
		, {ELLIPSE, "ELLIPSE"}
		, {REGULAR_POLYGON, "REGULAR_POLYGON"}
		, {RECTANGLE, "RECTANGLE"}
		, {TABLE, "TABLE"}
		, {TABLE_CELL, "TABLE_CELL"}
		, {TEXT, "TEXT"}
		, {TEXT_PATH, "TEXT_PATH"}
		, {SLICE, "SLICE"}
		, {COMPONENT, "COMPONENT"}
		, {COMPONENT_SET, "COMPONENT_SET"}
		, {INSTANCE, "INSTANCE"}
		, {STICKY, "STICKY"}
		, {SHAPE_WITH_TEXT, "SHAPE_WITH_TEXT"}
		, {CONNECTOR, "CONNECTOR"}
		, {WASHI_TAPE, "WASHI_TAPE"}
	};

	return s_mapTypeNamesTypeName.contains(eType) ? s_mapTypeNamesTypeName[eType] : "";
}

ENodeType StringToNodeType(const std::string& str)
{
	static std::map<std::string, ENodeType> s_mapTypeNamesNameType
	{
		{"DOCUMENT",DOCUMENT, }
		, {"CANVAS",CANVAS, }
		, {"FRAME",FRAME, }
		, {"GROUP",_GROUP}
		, {"TRANSFORM_GROUP",TRANSFORM_GROUP, }
		, {"SECTION",SECTION, }
		, {"VECTOR",VECTOR, }
		, {"BOOLEAN_OPERATION",BOOLEAN_OPERATION, }
		, {"STAR",STAR, }
		, {"LINE",LINE, }
		, {"ELLIPSE", ELLIPSE }
		, {"REGULAR_POLYGON", REGULAR_POLYGON}
		, {"RECTANGLE", RECTANGLE}
		, {"TABLE", TABLE}
		, {"TABLE_CELL", TABLE_CELL}
		, {"TEXT", TEXT}
		, {"TEXT_PATH", TEXT_PATH}
		, {"SLICE", SLICE}
		, {"COMPONENT", COMPONENT}
		, {"COMPONENT_SET", COMPONENT_SET}
		, {"INSTANCE", INSTANCE}
		, {"STICKY", STICKY}
		, {"SHAPE_WITH_TEXT", SHAPE_WITH_TEXT}
		, {"CONNECTOR", CONNECTOR}
		, {"WASHI_TAPE", WASHI_TAPE}
	};

	return s_mapTypeNamesNameType.contains(str) ? s_mapTypeNamesNameType[str] : _UNKNOWN;
}


ESliceFormat StringToSliceFormat(const std::string& str)
{
	static std::map<std::string, ESliceFormat> s_mapFormats
	{
		{"IGNORE", ESliceFormat_Ignore},
		{"SINGLE", ESliceFormat_Single},
		{"BUTTON_HORIZONTAL", ESliceFormat_ButtonHorizontal},
		{"BUTTON_VERTICAL", ESliceFormat_ButtonVertical},
		{"BUTTON_3x3", ESliceFormat_Button3x3},
		{"AUTO", ESliceFormat_ButtonAuto},
		{"REPORT_HEADER", ESliceFormat_ReportHeader},
		{"REPORT_GRID", ESliceFormat_ReportGrid},
	};

	return s_mapFormats.contains(str) ? s_mapFormats[str] : ESliceFormat_Unknown;
}


std::filesystem::path GetWorkingDirectory()
{
	return g_pathWorkingDirectory;
}


std::filesystem::path GetCacheDirectory()
{
	return GetWorkingDirectory() / "cache";
}


std::filesystem::path GetConfigFilePath()
{
	return (GetWorkingDirectory() / g_strConfigFileName).replace_extension("txt");
}


std::filesystem::path GetElementsCacheDirectory()
{
	return GetWorkingDirectory() / "cache/elements";
}


std::filesystem::path GetImagesCacheDirectory()
{
	return GetWorkingDirectory() / "cache/images";
}

std::filesystem::path GetOutputDirectory()
{
	return g_pathOutputDirectory;
}

std::string NormalizeElementName(const std::string& strElementName)
{
	std::string strResult = strElementName;
	if (auto pos = strResult.find("_3x1"); pos != strResult.npos)
		return strResult.erase(pos, strlen("_3x1"));

	else if (auto pos = strResult.find("_3x3"); pos != strResult.npos)
		return strResult.erase(pos, strlen("_3x3"));

	return strResult;
}

std::filesystem::path CreateOutputImagePath(const std::string& strElementName, const std::string& strSuffix, const float scale, const std::string& strFormat)
{
	const std::string strNormalizedName = NormalizeElementName(strElementName);

	auto strOutputName = strNormalizedName + strSuffix + (scale != 1.0f ? std::format("@{}x", scale) : "") + strFormat;
	return GetOutputDirectory() / strOutputName;
}

void SetOutputDirectory(std::filesystem::path path)
{
	if (!std::filesystem::exists(path))
		std::filesystem::create_directories(path);

	g_pathOutputDirectory = path;
}

void SetWorkingDirectory(std::filesystem::path path)
{
	g_pathWorkingDirectory = path;
}


std::string GetAccessToken()
{
	return g_strAccessToken;
}


bool IsRegExValid(const std::string& strRegEx)
{
	try
	{
		std::regex(strRegEx);
	}
	catch (std::regex_error) // if strRegEx contains invalid expession std::regex_error thrown
	{
		return false;
	}

	return true;
}

void SetAccessToken(std::string strToken)
{
	g_strAccessToken = strToken;
}


std::string GetFileDescriptor()
{
	return g_strFileDescriptor;
}

void SetFileDescriptor(std::string strDesc)
{
	g_strFileDescriptor = strDesc;
}

ShadowBorder::ShadowBorder(float left, float top, float right, float bottom)
	: m_fLeft{ left }
	, m_fTop{ top }
	, m_fRight{ right }
	, m_fBottom{ bottom }
{

}//ShadowBorder::ShadowBorder()

ShadowBorder::ShadowBorder(float f)
	: ShadowBorder(f,f,f,f)
{

}//ShadowBorder::ShadowBorder()


ShadowBorder& ShadowBorder::operator+=(const ShadowBorder& sBorderIncrement)
{
	m_fLeft += sBorderIncrement.m_fLeft;
	m_fRight += sBorderIncrement.m_fRight;
	m_fTop += sBorderIncrement.m_fTop;
	m_fBottom += sBorderIncrement.m_fBottom;
	return *this;
}//ShadowBorder::operator+=()


ShadowBorder& ShadowBorder::operator+=(float f)
{
	m_fLeft += f;
	m_fTop += f;
	m_fRight += f;
	m_fBottom += f;

	return *this;
}//ShadowBorder::operator+=


bool operator<(const ShadowBorder& left, const ShadowBorder& right)
{
	return left.m_fLeft < right.m_fLeft
		|| left.m_fTop < right.m_fTop
		|| left.m_fRight < right.m_fRight
		|| left.m_fBottom < right.m_fBottom;
}


ShadowBorder operator-(const ShadowBorder& left, const ShadowBorder& right)
{
	return {
		left.m_fLeft	- right.m_fLeft,
		left.m_fTop		- right.m_fTop,
		left.m_fRight	- right.m_fRight,
		left.m_fBottom	- right.m_fBottom
	};
}//operator-()


ShadowBorder operator*(const ShadowBorder& border, float f)
{
	return {
		border.m_fLeft * f,
		border.m_fTop * f,
		border.m_fRight * f,
		border.m_fBottom * f
	};
}//operator*()


ShadowBorder MaxBorder(const std::vector<ShadowBorder>& vecBorders)
{
	ShadowBorder result;

	for (const ShadowBorder& border : vecBorders)
	{
		result.m_fLeft = std::max(result.m_fLeft, border.m_fLeft);
		result.m_fTop = std::max(result.m_fTop, border.m_fTop);
		result.m_fRight = std::max(result.m_fRight, border.m_fRight);
		result.m_fBottom = std::max(result.m_fBottom, border.m_fBottom);
	}

	return result;
}

ShadowBorder CalcShadowBorderForElement(const rapidjson::Value& json)
{
	ShadowBorder borderResult;
	std::vector< ShadowBorder> vecDropShadows;
	std::vector< ShadowBorder> vecOutsideStrokes;

	if (json.HasMember("strokeAlign") && json["strokeAlign"].IsString() && json["strokeAlign"] == "OUTSIDE")
	{
		const float strokeWeight = json["strokeWeight"].GetFloat();
		if (json["strokeAlign"] == "OUTSIDE")
			vecOutsideStrokes.emplace_back(ShadowBorder{strokeWeight});
		
		else if (json["strokeAlign"] == "CENTER")
			vecOutsideStrokes.emplace_back(ShadowBorder{ strokeWeight * 0.5f });

	}
	if (json.HasMember("effects") && json["effects"].IsArray())
	{
		const rapidjson::Value& effects = json["effects"];
		for (auto it = effects.Begin(); it != effects.End(); it++)
		{
			const rapidjson::Value& effect = *it;

			if (effect["visible"].GetBool() && effect.HasMember("type") && effect["type"].IsString() && effect["type"].GetString() == std::string{"DROP_SHADOW"})
			{
				const float offset_x = effect["offset"]["x"].GetFloat();
				const float offset_y = effect["offset"]["y"].GetFloat();
				const float spread = effect.HasMember("spread") ? effect["spread"].GetFloat() : 0.0;
				float radius = effect["radius"].GetFloat();

				if (spread != 0.0)
					radius += spread;

				ShadowBorder border;

				border.m_fLeft += radius - offset_x;
				border.m_fTop += radius - offset_y;
				border.m_fRight += radius + offset_x;
				border.m_fBottom += radius + offset_y;

				vecDropShadows.emplace_back(border);
			}
		}
	}

	return MaxBorder({ MaxBorder(vecDropShadows),MaxBorder(vecOutsideStrokes) });
}