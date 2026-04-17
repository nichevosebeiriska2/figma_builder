#pragma once

#include <map>
#include <string>
#include <vector>
#include <filesystem>
#include <regex>

#include "document.h"

using UINT = unsigned int;
using IDs = std::pair<UINT, UINT>;
using TVecIDs = std::vector<IDs>;
using TVecStr = std::vector<std::string>;
using TVecFloat = std::vector<float>;
using TVecRegex = std::vector<std::regex>;
using TVecInts = std::vector<int>;

constexpr float			g_DefaultImageScale			= 2;
constexpr unsigned int	g_iNumOfImagesPerRequest	= 500;
constexpr unsigned int	g_iNumOfCurlHandlesParalle	= 20;
constexpr const char*	g_strConfigFileName			= "info";
constexpr const char*	g_strFigmaFileName			= "figFile.txt";
const inline std::string g_strBaseFigmaApiURL		= "https://api.figma.com/v1";

struct SHttpResponseData {
	std::string body;
	long status_code = 0;
};

struct ShadowBorder
{
	float m_fLeft{ 0.f };
	float m_fTop{ 0.f };
	float m_fRight{ 0.f };
	float m_fBottom{ 0.f };

	ShadowBorder() = default;
	ShadowBorder(float f);
	ShadowBorder(float fLeft, float fTop, float fRight, float fBottom);

	ShadowBorder& operator+=(const ShadowBorder& sBorderIncrement);
	ShadowBorder& operator+=(float f);
};

ShadowBorder operator*(const ShadowBorder& border, float f);
ShadowBorder operator-(const ShadowBorder& left, const ShadowBorder& right);
bool operator<(const ShadowBorder& left, const ShadowBorder& right);


enum ENodeType // all types described in api docs [https://developers.figma.com/docs/rest-api/].
{
	_UNKNOWN
	, DOCUMENT
	, CANVAS
	, FRAME
	, _GROUP
	, TRANSFORM_GROUP
	, SECTION
	, VECTOR
	, BOOLEAN_OPERATION
	, STAR
	, LINE
	, ELLIPSE
	, REGULAR_POLYGON
	, RECTANGLE
	, TABLE
	, TABLE_CELL
	, TEXT
	, TEXT_PATH
	, SLICE
	, COMPONENT
	, COMPONENT_SET
	, INSTANCE
	, STICKY
	, SHAPE_WITH_TEXT
	, CONNECTOR
	, WASHI_TAPE
};//ENodeType


enum ESliceFormat
{
	ESliceFormat_Unknown, 
	ESliceFormat_Ignore,

	ESliceFormat_Single,

	ESliceFormat_ButtonHorizontal,
	ESliceFormat_ButtonVertical,
	ESliceFormat_Button3x3,
	ESliceFormat_ButtonAuto,

	ESliceFormat_ReportHeader,
	ESliceFormat_ReportGrid,
};//ESliceFormat

ShadowBorder MaxBorder(const std::vector<ShadowBorder>& vecBorders);
ShadowBorder CalcShadowBorderForElement(const rapidjson::Value& json);
bool CheckElementHasGradient(rapidjson::Value& element_json);

std::string		ToString(ENodeType eType);
ENodeType		StringToNodeType(const std::string& str);
ESliceFormat	StringToSliceFormat(const std::string& str);

void SetAccessToken(std::string strToken);
void SetFileDescriptor(std::string strDesc);
void SetFileLibraryDescriptor(std::string strDesc);
void SetWorkingDirectory(std::filesystem::path path);
void SetOutputDirectory(std::filesystem::path path);

std::string GetAccessToken();
std::string GetFileDescriptor();
std::string GetLibraryFileDescriptor();
std::filesystem::path GetWorkingDirectory();
std::filesystem::path GetCacheDirectory();
std::filesystem::path GetConfigFilePath();
std::filesystem::path GetElementsCacheDirectory();
std::filesystem::path GetImagesCacheDirectory();
std::filesystem::path GetOutputDirectory();
std::filesystem::path CreateOutputImagePath(const std::string& strElementName, const std::string& strSuffix, const float scale, const std::string& strFormat = ".png");

std::regex TryCreateRegex(const std::string& strRegEx, bool& bSuccess);