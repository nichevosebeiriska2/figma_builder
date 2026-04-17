#pragma once

#include <vector>
#include <set>
#include "global.h"
#include "json_utils.h"

struct SDownloadTask {
	std::string url;
	std::string filepath;
};

bool CurlDownloadImagesAsync(const std::vector<SDownloadTask>& tasks);

// for map with key == image scale and value == set of element ids acynchroneous requesting links to scaled images
// for each scale responses collected in map 'responses' 
std::map<float, std::vector<rj::Document>> RequestImageLinksAsync(std::map<float, std::set<IDs>> mapScaleIds, std::map<float, std::vector<SHttpResponseData>>& responses);
rj::Document CurlGetFile();
rj::Document CurlGetLibraryFile();
rj::Document CurlGetFileInfoAsJsonDocument(); // get project json with depth == 1. If api/curl return invalid result - terminates proccess with error message
rj::Document CurlGetLibraryInfoAsJsonDocument(); // get library json with depth == 1. If api/curl return invalid result - terminates proccess with error message

IDs GetElementIDs(std::string strIDs);