#include "api_utils.h"

#include "curl.h"
#include "json_utils.h"
#include "CLogger.h"

#include <fstream>
#include <thread>
#include <list>
#include <algorithm>
#include <set>
#include <chrono>
#include <iostream>
#include <memory>
#include <queue>

const std::string g_strBaseURL = "https://api.figma.com/v1";

// curl write callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
	size_t total_size = size * nmemb;
	userp->append((char*)contents, total_size);
	return total_size;
}


size_t ImageWriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata) 
{	
	std::string strPath{ (const char*)userdata };
	std::ofstream file((const char*)userdata, std::ios::binary);

	if (file.is_open())
	{
		const char* p_image = (const char*)ptr;
		file.write(p_image, nmemb);

		return nmemb;
	}

	return 0;
}


std::string FormIdsStringFromContainer(const auto& container)
{
	std::string str_ids = "?ids=";

	for (const auto& [id_local, id_global] : container)
		str_ids += std::to_string(id_local) + ":" + std::to_string(id_global) + ",";

	str_ids.pop_back();

	return str_ids;
}


std::string FormFormat(EImageFormat eFmt)
{
	std::string str_format = "format=";
	switch (eFmt)
	{
		case(EImageFormatJPG):
			return str_format + "jpg";
		case(EImageFormatPNG):
			return str_format + "png";
		case(EImageFormatSVG):
			return str_format + "svg";
	}

	return "";
}


std::string FormScale(float scale)
{
	return std::string{ "scale=" } + std::to_string(scale);
}


std::string FormContentOnly()
{
	return "contents_only=true";
}


std::string CreateRequestHeader()
{
	return std::string{ "X-Figma-Token: " } + GetAccessToken();
}


CURL* CreateHandleToRequestImages(std::set<IDs> setIds, float scale, std::string& strBuffer)
{
	const std::string strIdsSequence = FormIdsStringFromContainer(setIds);

	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, CreateRequestHeader().c_str());

	std::string str_url = g_strBaseURL + "/images/" + GetFileDescriptor() + FormIdsStringFromContainer(setIds) + "&" + FormScale(scale) + "&" + FormFormat(EImageFormatPNG);

	CURL* curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, str_url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strBuffer);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	//curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	return curl;
}


struct SFigmaApiResponse
{
	rj::Document docResponse;
	int iReturnCode = 0;

	SFigmaApiResponse() = default;
	SFigmaApiResponse(const std::string& strResponse)
	{
		docResponse.Parse(strResponse.c_str());

		if (docResponse.HasMember("err") && !docResponse["err"].IsNull())
			iReturnCode = docResponse["status"].GetInt();
	}
};


struct SImageDownloadTask
{
	float scale;
	std::set<IDs> idsSet;
	SFigmaApiResponse apiResp;
	CURL* handle;
	std::string strResponseBuffer;

	void ParseFromBuffer()
	{
		apiResp = SFigmaApiResponse(strResponseBuffer);
	}
};


bool AsyncRequestDownloadTasks(const std::list<SImageDownloadTask>& tasks)
{
	CURLM* multi = curl_multi_init();
	if (!multi)
		return false;

	for (const auto& task : tasks)
		curl_multi_add_handle(multi, task.handle);

	int still_running = 0;
	do {
		CURLMcode mc = curl_multi_perform(multi, &still_running);
		if (mc != CURLM_OK)
			break;

		int numfds = 0;
		mc = curl_multi_poll(multi, nullptr, 0, 1000, &numfds);

		if (mc != CURLM_OK)
			break;

	} while (still_running);

	curl_multi_cleanup(multi);

	return true;
}


std::map<float, std::vector<rj::Document>> RequestImageLinksAsync(std::map<float, std::set<IDs>> mapScaleIds, std::map<float, std::vector<SHttpResponseData>>& responses)
{
	std::list<SImageDownloadTask> tasks; // нужен контейнер без переаллокации! иначе указатель на строку с которым создается хендл будет невалидным

	std::string strBuffer;
	for (auto [scale, setIDs] : mapScaleIds)
	{
		auto it = setIDs.begin();
		auto it_second = it;
		while (it != setIDs.end()) // sharding tasks to subtask with max size = g_iNumOfImagesPerRequest
		{
			// как будто можно найти контейнер получше чтобы не инкрементировать в цикле.
			while (std::distance(it, it_second) < g_iNumOfImagesPerRequest && it_second != setIDs.end())
				it_second++;
			
			tasks.emplace_back(SImageDownloadTask{ scale, std::set(it, it_second) });
			tasks.back().handle = CreateHandleToRequestImages(tasks.back().idsSet, scale, tasks.back().strResponseBuffer);
			
			it = it_second;
		}
	}

	std::map<float, std::vector<rj::Document>> mapResult;
	const int iCountRetries = 10;
	const int iSleepTimeSec = 30;

	const int iInitialTasksSize = tasks.size();
	Logger::PrintProgress("Requesting images...", iInitialTasksSize - tasks.size(), iInitialTasksSize);

	for (int i = 0;i < iCountRetries && !tasks.empty(); i++)
	{
		if (i && tasks.size())
		{
			Logger::Warning(std::format( "'{}' tasks rejected due to api limits. retrying in 30 seconds...", tasks.size()), 1);
			std::this_thread::sleep_for(std::chrono::seconds{ iSleepTimeSec });
		}

		AsyncRequestDownloadTasks(tasks);

		std::list<SImageDownloadTask> tasks_retry;
		for (auto& task : tasks)
		{
			task.ParseFromBuffer();
			
			const int iRetCode = task.apiResp.iReturnCode;
			if (iRetCode == 0)
				mapResult[task.scale].emplace_back(std::move(task.apiResp.docResponse));
			
			else if (iRetCode == 429) // rejected due to access limits
			{
				tasks_retry.emplace_back(SImageDownloadTask{ task.scale, std::move(task.idsSet) });
				tasks_retry.back().handle = CreateHandleToRequestImages(tasks.back().idsSet, task.scale, tasks_retry.back().strResponseBuffer);
			}
			else
				Logger::Error(std::format("API returned error code '{}'\n\t\t with error : '{}", iRetCode, task.strResponseBuffer));
			
		}

		std::swap(tasks, tasks_retry);
		Logger::PrintProgress("Requesting images...", iInitialTasksSize - tasks.size(), iInitialTasksSize);
	}

	return mapResult;
}


SHttpResponseData CurlGetImage(TVecIDs ids, float scale, bool bContentOnly)
{
	SHttpResponseData response;
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, CreateRequestHeader().c_str());

	std::string str_url = g_strBaseURL + "/images/" + GetFileDescriptor() + FormIdsStringFromContainer(ids) + "&" + FormScale(scale) + "&" + FormFormat(EImageFormatPNG);

	if (bContentOnly)
		str_url += "&" + FormContentOnly();

	CURL* curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, str_url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

	CURLcode res = curl_easy_perform(curl);

	return response;
}


SHttpResponseData CurlGetNode(TVecIDs ids)
{
	SHttpResponseData response;
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, CreateRequestHeader().c_str());

	std::string str_url = g_strBaseURL + "/files/" + GetFileDescriptor() + FormIdsStringFromContainer(ids);

	CURL* curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, str_url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

	CURLcode res = curl_easy_perform(curl);

	return response;
}


SHttpResponseData CurlGetFile()
{
	SHttpResponseData response;
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, CreateRequestHeader().c_str());

	std::string str_url = g_strBaseURL + "/files/" + GetFileDescriptor();

	CURL* curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, str_url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

	CURLcode res = curl_easy_perform(curl);

	return response;
}


SHttpResponseData CurlGetFileInfo()
{
	SHttpResponseData response;
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, CreateRequestHeader().c_str());

	std::string str_url = g_strBaseURL + "/files/" + GetFileDescriptor() + "?depth=1";

	CURL* curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, str_url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

	CURLcode res = curl_easy_perform(curl);

	return response;
}


bool CurlDownloadImagesAsync(const std::vector<DownloadTask>& tasks)
{
	CURLM* multi = curl_multi_init();
	if (!multi)
		return false;

	std::map<CURL*, size_t> handle_to_idx;
	std::vector<CURL*> easies;

	std::queue<CURL*> taskQueue;

	for (size_t i = 0; i < tasks.size(); ++i) {

		if (!std::filesystem::exists(tasks[i].filepath))
			std::filesystem::create_directories(std::filesystem::path{ tasks[i].filepath }.parent_path());

		CURL* easy = curl_easy_init();
		if (!easy) continue;

		curl_easy_setopt(easy, CURLOPT_URL, tasks[i].url.c_str());
		curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, ImageWriteCallback);
		curl_easy_setopt(easy, CURLOPT_WRITEDATA, tasks[i].filepath.c_str());
		curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(easy, CURLOPT_FAILONERROR, 1L);
		curl_easy_setopt(easy, CURLOPT_TIMEOUT, 300L);
		curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(easy, CURLOPT_USERAGENT, "libcurl-agent/1.0");

		taskQueue.push(easy);
		easies.push_back(easy);
		handle_to_idx[easy] = i;
	}

	int iCount = 0;
	const int iTasks = taskQueue.size();

	int still_running = 0;
	do
	{
		int iCount = 0;
		CURLMcode mc = curl_multi_perform(multi, &still_running);
		while (!taskQueue.empty() && iCount++ < (g_iNumOfCurlHandlesParalle - still_running))
		{
			CURLMcode mc = curl_multi_add_handle(multi, taskQueue.front());

			if (mc != CURLM_OK) 
				break;

			taskQueue.pop();
			Logger::PrintProgress("Downloading images", iTasks - taskQueue.size(), iTasks);
		}

		int numfds = 0;
		mc = curl_multi_poll(multi, nullptr, 0, 1000, &numfds);

	} while (!taskQueue.empty() || still_running);

	long http_code = 0;
	for (auto easy : easies)
	{
		curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &http_code);
		if (http_code != 200)
			Logger::Error(std::format("HTTP ret code : {}", http_code), 1);
	}
	
	for (CURL* easy : easies) {
		curl_multi_remove_handle(multi, easy);
		curl_easy_cleanup(easy);
	}
	curl_multi_cleanup(multi);

	return true;
}


bool CurlDownloadImage(std::filesystem::path pathToElement, std::string strImageURL)
{
	CURL* curl = curl_easy_init();

	std::string strPathToOutputFile = pathToElement.generic_string();

	if (!std::filesystem::exists(pathToElement.parent_path()))
		std::filesystem::create_directories(pathToElement.parent_path());

	SHttpResponseData res;

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, strImageURL.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ImageWriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, strPathToOutputFile.c_str());
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

		CURLcode res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		return res == CURLE_OK;
	}

	return false;
}


SApiResponse GetApiResponse(SHttpResponseData& response)
{
	rj::Document doc;
	doc.Parse(response.body.c_str());

	SApiResponse figApiResp;

	if(doc.HasMember("status"))
		return { doc["status"].GetInt(), doc["err"].GetString() };
	
	return { 0 };
}

// element can have pair of ids if format : "id1:id2" or two pairs : "local_id1:local_id2";"id1:id2"
// we need id1 id2 only to request image/node
IDs GetElementIDs(std::string strIDs)
{
	if (strIDs.contains(";")) // for component few ';' delimited pairs of id can appear. you need the last one
		strIDs = strIDs.substr(strIDs.rfind(";") + 1);
	
	int iPos = strIDs.find(':');

	std::string strIdFirst = strIDs.substr(0, iPos);
	std::string strIdSecond = strIDs.substr(iPos + 1, strIDs.length() - iPos - 1);

	return IDs{ std::atoi(strIdFirst.c_str()) , std::atoi(strIdSecond.c_str()) };
}