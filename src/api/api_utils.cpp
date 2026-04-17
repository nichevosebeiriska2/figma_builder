#include "api_utils.h"

#include "curl.h"
#include "global.h"
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



struct SLinksDownloadTask
{
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

std::string CreateRequestHeader()
{
	return std::string{ "X-Figma-Token: " } + GetAccessToken();
}//CreateRequestHeader()


// curl write callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
	size_t total_size = size * nmemb;
	userp->append((char*)contents, total_size);
	return total_size;
}//WriteCallback()


CURL* CurlCreateEasyHandle(const std::string& strUrl, SHttpResponseData& sResponse)
{
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, CreateRequestHeader().c_str());

	CURL* curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sResponse);

	return curl;
}//CurlCreateEasyHandle()


size_t ImageWriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata) 
{	
	std::ofstream file((const char*)userdata, std::ios::binary);

	if (file.is_open())
	{
		const char* p_image = (const char*)ptr;
		file.write(p_image, nmemb);

		return nmemb;
	}

	return 0; // curl handles zero as error
}//ImageWriteCallback()



std::string FormIdsStringFromContainer(const auto& container)
{
	std::string str_ids = "?ids=";

	for (const auto& [id_local, id_global] : container)
		str_ids += std::to_string(id_local) + ":" + std::to_string(id_global) + ",";

	str_ids.pop_back();

	return str_ids;
}//FormIdsStringFromContainer()


// creates CURL* handle to request set of images with specific scale
CURL* CurlCreateHandleToRequestImages(std::set<IDs> setIds, float scale, std::string& strBuffer)
{
	const std::string strIdsSequence = FormIdsStringFromContainer(setIds);

	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, CreateRequestHeader().c_str());

	std::string str_url = g_strBaseFigmaApiURL + "/images/" + GetFileDescriptor() + FormIdsStringFromContainer(setIds) + "&" + std::format("scale={}", scale) + "&format=png";

	CURL* curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, str_url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strBuffer);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	return curl;
}//CreateHandleToRequestImages()



bool AsyncRequestDownloadTasks(const std::list<SLinksDownloadTask>& tasks)
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
			return false;

		int numfds = 0;
		mc = curl_multi_poll(multi, nullptr, 0, 1000, &numfds);

		if (mc != CURLM_OK)
			return false;

	} while (still_running);

	curl_multi_cleanup(multi);

	return true;
}


std::map<float, std::vector<rj::Document>> RequestImageLinksAsync(std::map<float, std::set<IDs>> mapScaleIds, std::map<float, std::vector<SHttpResponseData>>& responses)
{
	std::list<SLinksDownloadTask> tasks; // нужен контейнер без переаллокации! иначе указатель на строку с которым создается хендл будет невалидным

	std::string strBuffer;
	for (auto [scale, setIDs] : mapScaleIds)
	{
		auto itRangeIdsFirst = setIDs.begin();
		auto itRangeIdsSecond = itRangeIdsFirst;
		while (itRangeIdsFirst != setIDs.end()) // sharding tasks to subtask with max size = g_iNumOfImagesPerRequest
		{
			// как будто можно найти контейнер получше чтобы не инкрементировать в цикле.
			while (std::distance(itRangeIdsFirst, itRangeIdsSecond) < g_iNumOfImagesPerRequest && itRangeIdsSecond != setIDs.end())
				itRangeIdsSecond++;
			
			tasks.emplace_back(SLinksDownloadTask{ scale, std::set(itRangeIdsFirst, itRangeIdsSecond) });
			tasks.back().handle = CurlCreateHandleToRequestImages(tasks.back().idsSet, scale, tasks.back().strResponseBuffer);
			
			itRangeIdsFirst = itRangeIdsSecond;
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

		if (!AsyncRequestDownloadTasks(tasks))
		{
			Logger::Error("RequestImageLinksAsync() - fatal curl error");
			std::terminate();
		}

		std::list<SLinksDownloadTask> tasks_retry;
		for (auto& task : tasks)
		{
			task.ParseFromBuffer();
			
			const int iRetCode = task.apiResp.iReturnCode;
			switch (iRetCode)
			{
				case(0):
				{
					mapResult[task.scale].emplace_back(std::move(task.apiResp.docResponse));
					break;
				}
				case(400):// fatal errors - 400 - invalid file descriptor, 404 - invalid parameters, 500 - internal server render error
				case(404):
				case(500):
				{
					Logger::Error(std::format("API returned error code '{}'\n\t\t with error : '{}", iRetCode, task.strResponseBuffer));
					Logger::Error("FATAL ERROR");
					std::terminate(); // clearly something went wrong. no reason to continue
				}
				case(429):
				{
					tasks_retry.emplace_back(SLinksDownloadTask{ task.scale, std::move(task.idsSet) });
					tasks_retry.back().handle = CurlCreateHandleToRequestImages(tasks.back().idsSet, task.scale, tasks_retry.back().strResponseBuffer);
					break;
				}
			}
		}

		if (tasks.size() == tasks_retry.size())
		{
			//iCountRetries--;
		}

		std::swap(tasks, tasks_retry);
		Logger::PrintProgress("Requesting images...", iInitialTasksSize - tasks.size(), iInitialTasksSize);
	}

	return mapResult;
}


rj::Document CurlGetFile()
{
	SHttpResponseData response;
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, CreateRequestHeader().c_str());

	std::string str_url = g_strBaseFigmaApiURL + "/files/" + GetFileDescriptor();

	CURL* curl = CurlCreateEasyHandle(str_url, response);

	if (!curl || curl_easy_perform(curl))
		Logger::Error("CurlGetfile() - curl error");

	curl_easy_cleanup(curl);

	rj::Document doc;
	doc.Parse(response.body.c_str());

	if (doc.IsNull() || doc.HasParseError())
	{
		Logger::Error("Fatal error - failed to get meta info for project");
		std::terminate();
	}

	return doc;
}//CurlGetFile()


SHttpResponseData CurlGetFileInfo(const std::string& strFileDescriptor)
{
	SHttpResponseData response;
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, CreateRequestHeader().c_str());

	std::string str_url = g_strBaseFigmaApiURL + "/files/" + strFileDescriptor + "?depth=1";

	CURL* curl = CurlCreateEasyHandle(str_url, response);

	if (!curl || curl_easy_perform(curl))
		Logger::Error("Failed to get file with curl");

	curl_easy_cleanup(curl);
	return response;
}

rj::Document CurlGetFileInfoAsJsonDocument()
{
	const SHttpResponseData sResponce = CurlGetFileInfo(GetFileDescriptor());
	
	if (sResponce.status_code)
	{
		Logger::Error(std::format("Error : CurlGetFileInfoAsJsonDocument() : http return code {}", sResponce.status_code));
		return rj::Document(rj::kNullType);
	}

	rj::Document doc;
	doc.Parse(sResponce.body.c_str());

	if (doc.IsNull() || doc.HasParseError())
	{
		Logger::Error("Fatal error - failed to get meta info for project");
		std::terminate();
	}

	return doc;
}

rj::Document CurlGetLibraryInfoAsJsonDocument()
{
	const SHttpResponseData sResponce = CurlGetFileInfo(GetLibraryFileDescriptor());

	if (sResponce.status_code)
	{
		Logger::Error(std::format("Error : CurlGetLibraryInfoAsJsonDocument() : http return code {}", sResponce.status_code));
		return rj::Document(rj::kNullType);
	}

	rj::Document doc;
	doc.Parse(sResponce.body.c_str());

	if (doc.IsNull() || doc.HasParseError())
	{
		Logger::Error("Fatal error - failed to get meta info for project");
		std::terminate();
	}

	return doc;
}


rj::Document CurlGetLibraryFile()
{
	SHttpResponseData response;
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, CreateRequestHeader().c_str());

	std::string str_url = g_strBaseFigmaApiURL + "/files/" + GetLibraryFileDescriptor();

	CURL* curl = CurlCreateEasyHandle(str_url, response);

	if (!curl || curl_easy_perform(curl))
		Logger::Error("CurlGetLibraryFile() - curl error");


	curl_easy_cleanup(curl);
	
	rj::Document doc;

	doc.Parse(response.body.c_str());

	if (doc.IsNull() || doc.HasParseError())
	{
		Logger::Error("Fatal error - failed to get meta info for project");
		std::terminate();
	}

	return doc;
}//CurlGetLibraryFile()


bool CurlDownloadImagesAsync(const std::vector<SDownloadTask>& tasks)
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
}//CurlDownloadImagesAsync()


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
}//GetElementIDs()