#include "log/log.h"
#include "github_manager.h"

int main(int argc, char *argv[])
{
    bool bUpdateRes = GithubHosts::instance()->updateGithubHosts();
    CONSOLE_INFO("update github hosts %s ...", bUpdateRes ? "success" : "failed");
#ifdef WIN32
    system("pause");
#endif
    return 0;
}