
#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include "curl/curl.h"
#include <string>
#include <memory>
#include <vector>

#define REQUEST_TIME_OUT 20L

class HttpClient
{
public:
	struct CurlGlobalInit
	{
		CurlGlobalInit()
		{
			curl_global_init(CURL_GLOBAL_ALL);
		}
		~CurlGlobalInit()
		{
			curl_global_cleanup();
		}
	};

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
	HttpClient(const std::string &url);
	~HttpClient();

	bool getResquestToFile(const std::string &filePath, const std::string &body = std::string(), bool isText = true);
	bool getResquestToFile(const std::string &filePath, const unsigned char *data = nullptr, unsigned int size = 0, bool isText = true);
	bool getResquestToStruct(ResponseData &responseData, const std::string &body = std::string());
	bool getResquestToStruct(ResponseData &responseData, const unsigned char*data = nullptr, unsigned int size = 0);
	void setContentType(const std::string &contentType);
	void addHeader(const std::string &header);
	void clearHeader();

private:
	enum DataMode
	{
		MEMORY_MODE = 0,
		FILE_MODE = 1,
	};

	static size_t writeCallbackFunc(void *data, size_t size, size_t nmemb, void *userp);
	CURLcode setOption(void *requestData, DataMode mode, const unsigned char *data, unsigned int size);
	std::string parseRedirectUrl(const std::string &url);

private:
	std::string m_url;
	CURL *m_curl;
	std::string m_contentType;
	std::vector<std::string> m_headers;
};

#endif