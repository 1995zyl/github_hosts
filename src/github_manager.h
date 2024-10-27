#ifndef __GITHUB_MANAGER_H__
#define __GITHUB_MANAGER_H__

#include "http_client/http_client.h"
#include "thread_pool/thread_pool.h"

class GithubHosts
{
public:
    static GithubHosts *instance();
    bool updateGithubHosts();

private:
    GithubHosts();
    ~GithubHosts();
    bool getUrlNames(std::vector<std::string> &urlNames);
    void getIpByUrl(std::vector<std::string> chunk);
    void parseIpByHtml(const std::vector<std::string>& suffixUrls);
    bool updateHostFile(std::vector<std::future<void>> &ipList);
    int getAverageTimeByPingIp(const std::string &ip);

private:
    std::unique_ptr<ThreadPool> m_threadPoolPtr;
    std::unordered_map<std::string, std::string> m_urlAndIps;
    std::mutex m_mutex;
    std::string m_tempDir;
};

#endif
