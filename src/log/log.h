#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <iostream>
#include <string>
#include <cstring>
#include <stdarg.h>
#include <thread>
#include <cassert>
#ifdef WIN32
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif
#include <memory>
#include <mutex>

class TimeHelper : public std::tm
{
public:
    std::string toString() const;
    std::string toStringForFilename() const;
    static TimeHelper currentTime();

private:
    TimeHelper();
    TimeHelper(const std::tm &_tm, int _tm_millisec, int _tm_microsec);

private:
    int tm_millisec; // 毫秒
    int tm_microsec; // 微秒
};

class Log
{
public:
    enum LOG_LEVEL
    {
        LOG_DEBUG = 0,
        LOG_INFO,
        LOG_WARN,
        LOG_ERROR,
        LOG_LEVEL_COUNT,
    };

    static Log *getInstance();
    void writeConsole(LOG_LEVEL logLevel, const char *format, ...);

private:
    Log();
    virtual ~Log();

private:
    std::unique_ptr<char[]> m_bufPtr;
    std::mutex m_mutex;
};

#define CONSOLE_DEBUG(format, ...) \
    Log::getInstance()->writeConsole(Log::LOG_DEBUG, format, ##__VA_ARGS__);

#define CONSOLE_INFO(format, ...) \
    Log::getInstance()->writeConsole(Log::LOG_INFO, format, ##__VA_ARGS__);

#define CONSOLE_WARN(format, ...) \
    Log::getInstance()->writeConsole(Log::LOG_WARN, format, ##__VA_ARGS__);

#define CONSOLE_ERROR(format, ...) \
    Log::getInstance()->writeConsole(Log::LOG_ERROR, format, ##__VA_ARGS__);

#endif
