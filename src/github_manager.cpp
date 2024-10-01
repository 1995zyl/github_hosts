#include "github_manager.h"
#include "log/log.h"
#include <fstream>
#include <regex>
#include <set>

namespace
{
#define GITHUB_URL "/github_url.txt"
#ifdef WIN32
#define HOSTS_FILE_PATH "C:/Windows/System32/drivers/etc/HOSTS"
#else
#define HOSTS_FILE_PATH "/etc/hosts"
#endif
#define PREFIX_HEAD "# GitHub Host Begin\n"
#define SUFFIX_TAIL "# GitHub Host End\n"
#define UPDATE_TIME "# Update Time: "

    const static std::string gBaseUrl("https://www.ipaddress.com/website/");
    const static std::string gSignAText("<h3>A<span> Records</span></h3>");
    const static std::string gSignAAAAText("<h3>AAAA<span> Records</span></h3>");
}

GithubHosts::GithubHosts()
    : m_threadPoolPtr(new ThreadPool(std::thread::hardware_concurrency()))
{
}

GithubHosts::~GithubHosts()
{
}

GithubHosts *GithubHosts::instance()
{
    static GithubHosts instance;
    return &instance;
}

bool GithubHosts::updateGithubHosts()
{
    std::vector<std::string> urlNames;
    if (!getUrlNames(urlNames))
    {
        CONSOLE_ERROR("url names is empty!");
        return false;
    }

    int taskCount = urlNames.size() / (m_threadPoolPtr->getThreadNum() + 1);
    std::vector<std::future<std::string>> htmlTextList;
    for (int i = taskCount; i < urlNames.size(); ++i)
    {
        htmlTextList.emplace_back(m_threadPoolPtr->enqueue(&GithubHosts::getIpByUrl, this, urlNames[i]));
    }

    if (!updateHostFile(taskCount, urlNames, htmlTextList))
    {
        CONSOLE_ERROR("update host file failed!");
        return false;
    }

    return true;
}

bool GithubHosts::getUrlNames(std::vector<std::string> &urlNames)
{
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
                urlNames.push_back(std::move(newLine));
        }
        file.close();
    }

    return !urlNames.empty();
}

std::string GithubHosts::getIpByUrl(const std::string &suffixUrl)
{
    std::unique_ptr<HttpClient> m_HttpClientPtr(new HttpClient(gBaseUrl + suffixUrl));
    HttpClient::ResponseData response;
    if (!m_HttpClientPtr->getResquestToStruct(response, ""))
        return "";

    return parseIpByHtml(response.response);
}

bool GithubHosts::updateHostFile(int taskCount, const std::vector<std::string> &urlNames,
                                 std::vector<std::future<std::string>> &htmlTextList)
{
    FILE *fp1 = fopen(HOSTS_FILE_PATH, "r");
    if (!fp1)
    {
        CONSOLE_ERROR("open file(read) error. file: %s", HOSTS_FILE_PATH);
        return false;
    }

    std::string tempFile = std::string(PROJECT_SOURCE_DIR) + "/temp.txt";
    FILE *fp2 = fopen(tempFile.c_str(), "w");
    if (!fp2)
    {
        CONSOLE_ERROR("open file(write) error. file: %s", tempFile.c_str());
        return false;
    }

    bool isInGithub = false;
    char buff[1024] = {0};
    while (fgets(buff, sizeof(buff) - 1, fp1) != nullptr)
    {
        if (strcmp(buff, PREFIX_HEAD) == 0)
            isInGithub = true;
        else if (strcmp(buff, SUFFIX_TAIL) == 0)
        {
            isInGithub = false;
            continue;
        }
        if (!isInGithub)
            fputs(buff, fp2);
    }

    TimeHelper nowTime = TimeHelper::currentTime();
    if (strcmp(buff, "\n") != 0)
        fputs("\n", fp2);
    fputs(PREFIX_HEAD, fp2);
    CONSOLE_INFO("start update gitHub Host ip address...");
    for (int i = 0; i < urlNames.size(); ++i)
    {
        std::string ipStr;
        if (i < taskCount)
            ipStr = getIpByUrl(urlNames[i]);
        else
            ipStr = htmlTextList[i - taskCount].get();
        if (!ipStr.empty())
        {
            ipStr.append(std::string(30 - ipStr.length(), ' '));
            ipStr.append(urlNames[i]);
            CONSOLE_INFO("%s", ipStr.c_str());
            ipStr.append("\n");
            fputs(ipStr.c_str(), fp2);
        }
    }
    std::string updateTime(UPDATE_TIME);
    updateTime.append(nowTime.toString());
    updateTime.append("\n");
    fputs(updateTime.c_str(), fp2);
    fputs(SUFFIX_TAIL, fp2);

    fclose(fp1);
    fclose(fp2);

    remove(HOSTS_FILE_PATH);
    return !rename(tempFile.c_str(), HOSTS_FILE_PATH);
}

std::string GithubHosts::parseIpByHtml(const std::string &htmlText)
{
    if (htmlText.empty())
        return "";

    size_t indexA = htmlText.find(gSignAText);
    size_t indexAAAA = htmlText.find(gSignAAAAText, indexA);
    if (indexA == std::string::npos || indexAAAA == std::string::npos)
    {
        CONSOLE_ERROR("failed to get sign text(%s, %s) from html text", gSignAText.c_str(), gSignAAAAText.c_str());
        return "";
    }

    std::string subHtmlText = htmlText.substr(indexA, indexAAAA - indexA);
    std::vector<std::string> ipList;
    std::regex pattern("((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)");
    std::smatch result;
    std::string::const_iterator iterStart = subHtmlText.begin();
    std::string::const_iterator iterEnd = subHtmlText.end();
    while (std::regex_search(iterStart, iterEnd, result, pattern))
    {
        std::string ip = result[0];
        ipList.emplace_back(ip);
        iterStart = result[0].second;
    }

    if (ipList.empty())
        return "";

    std::string bestIp;
    int minPingTime = INT_MAX;
    for (int i = 0; i < ipList.size(); ++i)
    {
        int pingTime = getAverageTimeByPingIp(ipList[i]);
        if (pingTime < minPingTime)
        {
            bestIp = ipList[i];
            minPingTime = pingTime;
        }
    }
    return bestIp;
}

int GithubHosts::getAverageTimeByPingIp(const std::string &ip)
{
    std::string pingStr("ping -n 2 ");
    pingStr.append(ip);
#ifdef WIN32
    FILE *pipe = _popen(pingStr.c_str(), "r");
#else
    FILE *pipe = popen(pingStr.c_str(), "r");
#endif
    if (!pipe)
        return INT_MAX;

    std::vector<char> buffer(256);
    std::string pingInfo;
    while (!feof(pipe))
    {
        if (fgets(buffer.data(), buffer.size(), pipe))
            pingInfo.append(buffer.data());
    }
#ifdef WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    size_t index = pingInfo.find_last_of(" = ");
    if (std::string::npos == index)
        return INT_MAX;

    std::string subStr = pingInfo.substr(index);
    subStr.erase(subStr.end() - 3, subStr.end());
    int pingTime = atoi(subStr.c_str());
    return pingTime;
}