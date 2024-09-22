#include "github_hosts.h"
#include "log/log.h"
#include <fstream>
#include <regex>


namespace
{
#define GITHUB_URL    "/src/github_url.txt"
#ifdef WIN32
#define HostsFilePath "C:/Windows/System32/drivers/etc/HOSTS"
#else
#define HostsFilePath "/etc/hosts"
#endif
#define PrefixHead "# GitHub Host Start\n"
#define PrefixTail "# GitHub Host End\n"

const static std::string gBaseUrl("https://www.ipaddress.com/website/");
}


GithubHosts::GithubHosts()
    : m_threadPoolPtr(new ThreadPool(std::thread::hardware_concurrency()))
{

}

GithubHosts::~GithubHosts()
{

}

GithubHosts* GithubHosts::instance()
{
    static GithubHosts instance;
    return &instance;
}

bool GithubHosts::updateGithubHosts()
{
    if(!getUrlNames())
    {
        CONSOLE_ERROR("url names is empty!");
        return false;
    }

    for (auto url : m_urlNames)
    {
        m_urlAndIps[url] = m_threadPoolPtr->enqueue(&GithubHosts::getIpByUrl, this, url);
    }

    if (!updateHostFile())
    {
        CONSOLE_ERROR("update host file failed!");
        return false;
    }

    return true;
}

bool GithubHosts::getUrlNames()
{
    m_urlNames.clear();
    std::regex regex("\\s|\n");
    std::string github_url_path(std::string(PROJECT_SOURCE_DIR) + GITHUB_URL);
    std::ifstream file(github_url_path);
    if (file.is_open()) 
    {
        std::string line;
        while (std::getline(file, line))
        {
            std::string newLine = std::regex_replace(line, regex, "");
            if (!newLine.empty())
                m_urlNames.push_back(std::move(newLine));
        }
        file.close();
    }

    return !m_urlNames.empty();
}

std::string GithubHosts::getIpByUrl(const std::string& suffixUrl)
{
    std::unique_ptr<HttpClient> m_HttpClientPtr(new HttpClient(gBaseUrl + suffixUrl));
    HttpClient::ResponseData response;
    if (!m_HttpClientPtr->getResquestToStruct(response, ""))
    {
        CONSOLE_ERROR("get resquest to struct failed.");
        return "";
    }

    return std::string(response.response);
}

bool GithubHosts::updateHostFile()
{
    FILE* fp1 = fopen(HostsFilePath, "r");
    if (!fp1)
    {
        CONSOLE_ERROR("open file(read) error. file: %s", HostsFilePath);
        return false;
    }

    // std::string tempFile = std::string(HostsFilePath) + "_temp";
    std::string tempFile = "D:\\AA\\project\\GitHub\\github_hosts\\temp";
    FILE* fp2 = fopen(tempFile.c_str(), "w");
    if (!fp2)
    {
        CONSOLE_ERROR("open file(write) error. file: %s", tempFile.c_str());
        return false;
    }

    bool isInGithub = false;
    char buff[1024] = { 0 };
    while (fgets(buff, sizeof(buff) - 1, fp1) != nullptr)
    {
        if (strcmp(buff, PrefixHead) == 0)
            isInGithub = true;
        else if (strcmp(buff, PrefixTail) == 0)
        {
            isInGithub = false;
            continue;
        }
        if (!isInGithub)
            fputs(buff, fp2);
    }

#ifdef WIN32
    fputs("\n", fp2);
#endif
    fputs(PrefixHead, fp2);
    for (auto iter = m_urlAndIps.begin(); iter != m_urlAndIps.end(); ++iter)
    {
        std::string ipStr = parseIpByHtml(iter->second.get());
        if (!ipStr.empty())
            fputs(ipStr.c_str(), fp2);
    }
    fputs(PrefixTail, fp2);
    fclose(fp1);
    fclose(fp2);
    //remove(HostsFilePath);
    //rename(tempFile.c_str(), HostsFilePath);

    return true;
}

std::string GithubHosts::parseIpByHtml(const std::string& htmlText)
{
    std::vector<std::string> ipList;
    std::regex pattern("((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)");
    std::smatch result;
    std::string::const_iterator iterStart = htmlText.begin();
    std::string::const_iterator iterEnd = htmlText.end();
    while (std::regex_search(iterStart, iterEnd, result, pattern))
    {
        std::string ip = result[0];
        ipList.push_back(ip);
        iterStart = result[0].second;
    }

    if (ipList.empty())
        return "";

    return ipList[0];

}
