#include "log.h"
#include <sstream>
#include <ctime>
#include <chrono>
#ifdef WIN32
#include <Windows.h>
#endif


namespace
{
#define RESET "\033[0m"
#define BLACK "\033[30m"              /* Black */
#define RED "\033[31m"                /* Red */
#define GREEN "\033[32m"              /* Green */
#define YELLOW "\033[33m"             /* Yellow */
#define BLUE "\033[34m"               /* Blue */
#define MAGENTA "\033[35m"            /* Magenta */
#define CYAN "\033[36m"               /* Cyan */
#define WHITE "\033[37m"              /* White */
#define BOLDBLACK "\033[1m\033[30m"   /* Bold Black */
#define BOLDRED "\033[1m\033[31m"     /* Bold Red */
#define BOLDGREEN "\033[1m\033[32m"   /* Bold Green */
#define BOLDYELLOW "\033[1m\033[33m"  /* Bold Yellow */
#define BOLDBLUE "\033[1m\033[34m"    /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m" /* Bold Magenta */
#define BOLDCYAN "\033[1m\033[36m"    /* Bold Cyan */
#define BOLDWHITE "\033[1m\033[37m"   /* Bold White */

#define LOG_BUFFER_SIZE     2048

class TimeHelper : public std::tm
{
public:
    std::string toString() const
    {
        char temp[27]{0};
        snprintf(temp,
                 27,
                 "%04d-%02d-%02d %02d:%02d:%02d.%03d%03d",
                 tm_year + 1900,
                 tm_mon + 1,
                 tm_mday,
                 tm_hour,
                 tm_min,
                 tm_sec,
                 tm_millisec,
                 tm_microsec);
        return std::string(temp);
    }

    std::string toStringForFilename() const
    {
        char temp[23]{0};
        snprintf(temp,
                 23,
                 "%04d%02d%02d_%02d%02d%02d.%03d%03d",
                 tm_year + 1900,
                 tm_mon + 1,
                 tm_mday,
                 tm_hour,
                 tm_min,
                 tm_sec,
                 tm_millisec,
                 tm_microsec);
        return std::string(temp);
    }

    static TimeHelper currentTime()
    {
        // 从1970-01-01 00:00:00到当前时间点的时长
        auto duration_since_epoch = std::chrono::system_clock::now().time_since_epoch();

        // 将时长转换为微秒数
        auto microseconds_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(duration_since_epoch).count();

        // 将时长转换为秒数
        time_t seconds_since_epoch = static_cast<time_t>(microseconds_since_epoch / 1000000);

#if defined _MSC_VER && _MSC_VER >= 1400
        TimeHelper exact_time;
        localtime_s(&exact_time, &seconds_since_epoch);
        exact_time.tm_microsec = static_cast<int>(microseconds_since_epoch % 1000);
        exact_time.tm_millisec = static_cast<int>(microseconds_since_epoch / 1000 % 1000);
#elif defined __GNUC__
        TimeHelper exact_time;
        localtime_r(&seconds_since_epoch, &exact_time);
        exact_time.tm_microsec = static_cast<int>(microseconds_since_epoch % 1000);
        exact_time.tm_millisec = static_cast<int>(microseconds_since_epoch / 1000 % 1000);
#else
#error "Unknown compiler"
#endif

        return exact_time;
    }

private:
    TimeHelper()
        : tm_millisec(0), tm_microsec(0)
    {
    }
    TimeHelper(const std::tm &_tm, int _tm_millisec, int _tm_microsec)
        : std::tm(_tm), tm_millisec(_tm_millisec), tm_microsec(_tm_microsec)
    {
    }

    int tm_millisec; // 毫秒
    int tm_microsec; // 微秒
};

static char gConsoleColors[Log::LOG_LEVEL_COUNT][256] =
{
    BLUE,
    GREEN,
    MAGENTA,
    RED
};

}

Log::Log() 
    : m_bufPtr(new char[LOG_BUFFER_SIZE]{0})
{
}

Log::~Log()
{
}

Log *Log::getInstance()
{
    static Log gLogInstance;
    return &gLogInstance;
}

void Log::writeConsole(LOG_LEVEL logLevel, const char *format, ...)
{
    TimeHelper nowTime = TimeHelper::currentTime();

    va_list valst;
    va_start(valst, format);

    {
        std::lock_guard<std::mutex> lg(m_mutex);
        int n = snprintf(m_bufPtr.get(), LOG_BUFFER_SIZE - 1, "%s ", nowTime.toString().c_str());
        int m = vsnprintf(m_bufPtr.get() + n, LOG_BUFFER_SIZE - n - 1, format, valst);
        assert(n + m + 1 < LOG_BUFFER_SIZE);
        std::cout << gConsoleColors[(int)logLevel] << m_bufPtr.get() << RESET << std::endl;
    }

    va_end(valst);
}

