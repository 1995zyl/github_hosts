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
    bool getUrlNames(std::vector<std::string>& urlNames);
    std::string getIpByUrl(const std::string& suffixUrl);
    bool updateHostFile(int taskCount, const std::vector<std::string>& urlNames, 
        std::vector<std::future<std::string>>& htmlTextList);
    std::string parseIpByHtml(const std::string& htmlText);
    int getAverageTimeByPingIp(const std::string& ip);

private:
    std::unique_ptr<ThreadPool> m_threadPoolPtr;
};

#endif
