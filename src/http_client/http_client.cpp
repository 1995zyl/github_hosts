#include "http_client.h"
#include "assert.h"
#include "log/log.h"


const static std::string gDefaultContentType("application/json");

HttpClient::HttpClient(const std::string &url)
    : m_url(url), m_curl(curl_easy_init()), m_contentType(gDefaultContentType)
{
    assert(!m_url.empty());
    assert(m_curl);

    static CurlGlobalInit gCurlGlobalInit;
}

HttpClient::~HttpClient()
{
    curl_easy_cleanup(m_curl);
}

bool HttpClient::getResquestToFile(const std::string &filePath, const std::string &body, bool isText)
{
    return getResquestToFile(filePath, reinterpret_cast<const unsigned char*>(body.data()), body.size(), isText); //todo char ת byte ����������
}

bool HttpClient::getResquestToFile(const std::string &filePath, const unsigned char *data, unsigned int size, bool isText)
{
    FILE *fp = fopen(filePath.c_str(), isText ? "w" : "wb");
    if (!fp)
    {
        CONSOLE_ERROR("open %s error!", filePath.c_str());
        return false;
    }

    CURLcode res = setOption(static_cast<void *>(fp), FILE_MODE, data, size);

    fclose(fp);
    return !res;
}

bool HttpClient::getResquestToStruct(ResponseData &responseData, const std::string &body)
{
    return getResquestToStruct(responseData, reinterpret_cast<const unsigned char *>(body.data()), body.size()); //todo char ת byte ����������
}

bool HttpClient::getResquestToStruct(ResponseData &responseData, const unsigned char *data, unsigned int size)
{
    return !setOption(static_cast<void *>(&responseData), MEMORY_MODE, data, size);
}

void HttpClient::setContentType(const std::string &contentType)
{
    m_contentType = contentType;
}

void HttpClient::addHeader(const std::string &header)
{
    m_headers.emplace_back(header);
}

void HttpClient::clearHeader()
{
    m_headers.clear();
}

CURLcode HttpClient::setOption(void *responseData, DataMode mode, const unsigned char *data, unsigned int size)
{
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Accept:application/json");
    headers = curl_slist_append(headers, (std::string("Content-Type:") + m_contentType).c_str());
    headers = curl_slist_append(headers, "charset:utf-8");
    for (int i = 0; i < m_headers.size(); ++i)
        headers = curl_slist_append(headers, m_headers[i].c_str());

    curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
    if (data && size > 0)
    {
        curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, size);
    }
    if (MEMORY_MODE == mode)
        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, HttpClient::writeCallbackFunc);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, responseData);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, REQUEST_TIME_OUT);

    CURLcode res = curl_easy_perform(m_curl);
    if (CURLE_OK != res)
    {
        CONSOLE_ERROR("curl_easy_perform failed: %s, url: %s", curl_easy_strerror(res), m_url.c_str());
    }
    else
    {
        long responseCode;
        curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &responseCode);
        switch (responseCode)
        {
        case 200:
            break;
        case 302:
        {
            struct ResponseData *rd = static_cast<struct ResponseData *>(responseData);
            assert(rd);
            m_url = parseRedirectUrl(rd->response);
            setOption(responseData, mode, data, size);
            break;
        }
        default:
            CONSOLE_ERROR("curl_easy_perform error. responseCode: %ld, url: %s", responseCode, m_url.c_str());
            break;
        }
    }

    curl_slist_free_all(headers);
    return res;
}

size_t HttpClient::writeCallbackFunc(void *data, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct ResponseData *mem = static_cast<struct ResponseData *>(userp);
    char *ptr = static_cast<char *>(realloc(mem->response, mem->size + realsize + 1));
    if (!ptr)
    {
        CONSOLE_ERROR("realloc failed, out of memory!");
        return 0;
    }
    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;
    return realsize;
}

std::string HttpClient::parseRedirectUrl(const std::string &url)
{
    if (url.find_first_of("href") == -1)
        return url;
    int beginIndex = url.find("\"");
    int endIndex = url.find("\">");
    return url.substr(beginIndex + 1, endIndex - beginIndex - 1);
}
