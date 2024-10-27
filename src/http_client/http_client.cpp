#include "http_client.h"
#include "assert.h"
#include "log/log.h"
#include <unordered_map>
#include <algorithm>
#include <filesystem>

namespace
{
#define REQUEST_TIME_OUT  20L
const static std::string gDefaultContentType("application/json");
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

void curlGlobalInit()
{
    static CurlGlobalInit gCurlGlobalInit;
}

class CurlSlist
{
public:
    CurlSlist(const std::string& contentType, const std::vector<std::string> m_headers)
    {
        m_curlSlist = curl_slist_append(m_curlSlist, "Accept:application/json");
        m_curlSlist = curl_slist_append(m_curlSlist, (std::string("Content-Type:") + contentType).c_str());
        m_curlSlist = curl_slist_append(m_curlSlist, "charset:utf-8");
        for (int i = 0; i < m_headers.size(); ++i) {
            m_curlSlist = curl_slist_append(m_curlSlist, m_headers[i].c_str());
        }
    }

    ~CurlSlist()
    {
        curl_slist_free_all(m_curlSlist);
    }

    struct curl_slist* getCurlSlist() { return m_curlSlist; }
private:
    struct curl_slist *m_curlSlist = nullptr;
};
}

HttpClient::HttpClient(const std::vector<std::string> &urls)
    : m_urls(urls), m_multiCurl(curl_multi_init()), m_contentType(gDefaultContentType)
{
    curlGlobalInit();
}

HttpClient::HttpClient(const std::string& url)
    : HttpClient(std::vector<std::string>{ url })
{

}

HttpClient::~HttpClient()
{
    curl_multi_cleanup(m_multiCurl);
}

void HttpClient::setContentType(const std::string& contentType)
{
    m_contentType = contentType;
}

void HttpClient::setPostField(const std::string& postField)
{
    m_postField = postField;
}

void HttpClient::addHeader(const std::string& header)
{
    m_headers.emplace_back(header);
}

void HttpClient::clearHeader()
{
    m_headers.clear();
}

bool HttpClient::getResponse(ResponseData &response)
{
    if (m_urls.size() != 1)
        return false;
    
    std::vector<void *> responses{ reinterpret_cast<void *>(&response) };
    bool result = false;
    setOption(responses, MEMORY_MODE, [&result](int index, bool bOk) { result = bOk; });
    return result;
}

bool HttpClient::awaitResponse(
    std::function<void(const std::string&, bool, const ResponseData&)> callback)
{
    std::vector<ResponseData> responses(m_urls.size());
    std::vector<void *> responses_;
    std::for_each(responses.begin(), responses.end(), [&responses_](ResponseData& response) {
        responses_.emplace_back(reinterpret_cast<void*>(&response)); });

    setOption(responses_, MEMORY_MODE, [callback, this, &responses](int index, bool bOk) {
        if (!callback)
            return;
        callback(m_urls[index], bOk, responses[index]);
    });

    return true;
}

bool HttpClient::getResponseToFile(const std::string& filePath, bool isText)
{
    if (m_urls.size() != 1)
        return false;

    FILE* fp = fopen(filePath.c_str(), isText ? "w" : "wb");
    if (!fp)
    {
        CONSOLE_ERROR("open %s error!", filePath.c_str());
        return false;
    }

    bool result = false;
    std::vector<void*> responses{ reinterpret_cast<void*>(fp) };
    setOption(responses, FILE_MODE, [&result](int index, bool bOk) { result = bOk; });
    fclose(fp);
    return result;
}

bool HttpClient::awaitResponseToFile(const std::string& fileDir, 
    std::function<void(const std::string&, bool)> callback, bool isText)
{
    std::vector<void*> responses;
    for (const auto& url : m_urls)
    {
        size_t pos = url.find_last_of('/');
        if (pos == std::string::npos)
        {
            CONSOLE_ERROR("url is invalid", url.c_str());
            return false;
        }
        
        std::string filePath;
        std::string suffixUrl = url.substr(pos + 1);
        if (fileDir.back() != '/')
            filePath = fileDir + "/" + suffixUrl;
        else
            filePath = fileDir + suffixUrl;
        
        FILE* fp = fopen(filePath.c_str(), isText ? "w" : "wb");
        if (!fp)
        {
            CONSOLE_ERROR("open %s error!", filePath.c_str());
            return false;
        }

        responses.emplace_back(static_cast<void*>(fp));
    }

    setOption(responses, FILE_MODE, [callback, this](int index, bool bOk){
        if (!callback)
            return;
        callback(m_urls[index], bOk);
    });

    std::for_each(responses.begin(), responses.end(), [](void *fp) {
        fclose(static_cast<FILE *>(fp)); });
    
    return true;
}

void HttpClient::setOption(const std::vector<void *> &responses, DataMode mode, std::function<void(int, bool)> callback)
{
    CurlSlist curlSlist(m_contentType, m_headers);
    std::unordered_map<CURL*, int> curlAndIndexs;
    for (int i = 0; i < m_urls.size(); ++i) {
        CURL* curl = curl_easy_init();
        curlAndIndexs.insert({curl, i});

        assert(CURLE_OK == curl_easy_setopt(curl, CURLOPT_URL, m_urls[i].c_str()));
        assert(CURLE_OK == curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0));
        assert(CURLE_OK == curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curlSlist.getCurlSlist()));
        if (!m_postField.empty()){
            assert(CURLE_OK == curl_easy_setopt(curl, CURLOPT_POST, 1L));
            assert(CURLE_OK == curl_easy_setopt(curl, CURLOPT_POSTFIELDS, reinterpret_cast<const unsigned char *>(m_postField.data())));
            assert(CURLE_OK == curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, m_postField.size()));
        }
        if (MEMORY_MODE == mode)
            assert(CURLE_OK == curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpClient::writeCallbackFunc));
        assert(CURLE_OK == curl_easy_setopt(curl, CURLOPT_WRITEDATA, responses[i]));
        assert(CURLE_OK == curl_easy_setopt(curl, CURLOPT_TIMEOUT, REQUEST_TIME_OUT));

        curl_multi_add_handle(m_multiCurl, curl);
    }

    int still_running = 0;
    do 
    {
        curl_multi_perform(m_multiCurl, &still_running);
        CURLMsg* msg = nullptr;
        int msgs_left = -1;
        while ((msg = curl_multi_info_read(m_multiCurl, &msgs_left))) {
            bool bOk = false;
            CURL* curl = msg->easy_handle;
            if (msg->msg == CURLMSG_DONE) {
                if (curlAndIndexs.end() != curlAndIndexs.find(curl)) {
                    if (CURLE_OK != msg->data.result) {
                        CONSOLE_ERROR("curl_multi_info_read failed, url: %s, errorcode: %d", m_urls[curlAndIndexs[curl]].c_str(), msg->data.result);
                        continue;
                    }
                    bOk = true;
                }
                else {
                    assert(false);
                }
            }
            else {
                CONSOLE_ERROR("curl_multi_info_read failed, msg: %d", msg->msg);
            }

            if (callback) {
                callback(curlAndIndexs[curl], bOk);
            }
        }
    } while (still_running);

    for (const auto& it : curlAndIndexs) {
        curl_multi_remove_handle(m_multiCurl, it.first);
        curl_easy_cleanup(it.first);
    }
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
