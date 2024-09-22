#ifndef  __GITHUB_HOSTS_H__
#define  __GITHUB_HOSTS_H__

#include "http_client/http_client.h"
#include "thread_pool/thread_pool.h"

class GithubHosts
{
public:
    static GithubHosts* instance();
    bool updateGithubHosts();

private:
    GithubHosts();
    ~GithubHosts();
    bool getUrlNames();
    std::string getIpByUrl(const std::string& suffixUrl);
    bool updateHostFile();
    std::string parseIpByHtml(const std::string& htmlText);

private:
    std::unique_ptr<ThreadPool> m_threadPoolPtr;
    std::vector<std::string> m_urlNames;
    std::unordered_map<std::string, std::future<std::string>> m_urlAndIps;
};

#endif
