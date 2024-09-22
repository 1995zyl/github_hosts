#include "log/log.h"
#include "github_hosts.h"


int main(int argc, char *argv[])
{
    bool bUpdateRes = GithubHosts::instance()->updateGithubHosts();
    CONSOLE_INFO("update github hosts %s ...", bUpdateRes ? "success" : "failed");
    system("pause");
    return 0;
}