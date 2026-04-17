#pragma once

#include <vector>
#include <set>
//#include <curl.h>
#include "global.h"
#include "json_utils.h"

struct DownloadTask {
	std::string url;
	std::string filepath;
};

bool CurlDownloadImagesAsync(const std::vector<DownloadTask>& tasks);

std::map<float, std::vector<rj::Document>> RequestImageLinksAsync(std::map<float, std::set<IDs>> mapScaleIds, std::map<float, std::vector<SHttpResponseData>>& responses);
SHttpResponseData CurlGetImage(TVecIDs ids, float scale, bool bContentOnly);
SHttpResponseData CurlGetNode(TVecIDs ids);
SHttpResponseData CurlGetFile();
SHttpResponseData CurlGetFileInfo(); // get top level of figma file to check its metadata
bool CurlDownloadImage(std::filesystem::path pathToElement, std::string strImageURL);

//std::pair<std::string, std::string> ComponentNameToItsMainPartAndStateSuffix(const std::string& strComponentName);
//std::filesystem::path ComponentNameToPath(const std::string& strComponentName);

struct SApiResponse
{
	int m_iCode = 0;
	std::string m_strError;
};

SApiResponse GetApiResponse(SHttpResponseData& response);
IDs GetElementIDs(std::string strIDs);

//bool CheckFileNeedUpdate();