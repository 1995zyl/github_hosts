
#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include "curl/curl.h"
#include <string>
#include <memory>
#include <vector>
#include <functional>


class HttpClient
{
public:
	struct ResponseData
	{
		ResponseData() : response(nullptr), size(0)
		{
		}
		~ResponseData()
		{
			if (response && size)
			{
				delete[] response;
				size = 0;
			}
		}
		char *response;
		size_t size;
	};

public:
	HttpClient(const std::vector<std::string> &urls);
	HttpClient(const std::string& url);
	~HttpClient();

	bool getResponse(ResponseData &response);
	bool awaitResponse(std::function<void(const std::string&, bool, const ResponseData&)> callback);
	bool getResponseToFile(const std::string& filePath, bool isText = true);
	bool awaitResponseToFile(const std::string& fileDir, std::function<void(const std::string&, bool)> callback, bool isText = true);
	void setContentType(const std::string &contentType);
	void setPostField(const std::string &postField);
	void addHeader(const std::string &header);
	void clearHeader();

private:
	enum DataMode
	{
		MEMORY_MODE = 0,
		FILE_MODE = 1,
	};

	static size_t writeCallbackFunc(void *data, size_t size, size_t nmemb, void *userp);
	void setOption(const std::vector<void *> &requestDatas, DataMode mode, std::function<void(int, bool)> callback);

private:
	std::vector<std::string> m_urls;
	CURLM* m_multiCurl = nullptr;
	std::string m_contentType;
	std::string m_postField;
	std::vector<std::string> m_headers;
};

#endif