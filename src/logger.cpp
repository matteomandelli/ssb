// SSB, v0.01 WIP
// (Logger)

#include "logger.hpp"

#include <stdarg.h>
#include <ctype.h>

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <sstream>

#ifdef _WINDOWS
    #include <windows.h>
    #pragma warning(disable:4996) // _CRT_SECURE_NO_WARNINGS
#endif

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define MAX_FILE_SIZE       100*1024*1024

namespace {

Log::Level      s_logLevel = Log::kLogError;
const char*     s_logFileName = nullptr;
std::ofstream   s_logFile;

void Rotate(const char* file)
{
    if (s_logFile.is_open())
    {
        s_logFile.flush();
        s_logFile.close();
    }

    if (s_logFileName != nullptr)
    {
        std::stringstream filename;
        time_t rawtime = time(NULL);
        struct tm* t;
        t = localtime(&rawtime);
        filename << t->tm_year + 1900 << "-" << t->tm_mon + 1 << "-" << t->tm_mday << "_" << t->tm_hour << "-" << t->tm_min << "_" << file;
        s_logFile.open(filename.str().c_str());
    }
}

} // namespace

namespace Log {

int Initialise(Level level, const char* file)
{
    s_logLevel = level;
    s_logFileName = file;
    Rotate(file);
    return 0;
}

int Terminate(void) {
    if (s_logFileName != nullptr)
    {
        s_logFile.flush();
        s_logFile.close();
    }
    return 0;
}

void Log(Level level, bool compact, const char* appName, const char* format, ...)
{
    if (level > s_logLevel)
        return;

    char buf[4096];
    memset(buf, 0, 4096);

    size_t pos = 0;
    
    if (!compact)
    {
        time_t rawtime = time(NULL);
        struct tm* t;
        t = localtime(&rawtime);
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        ms -= std::chrono::milliseconds(rawtime*1000);

#ifndef _WINDOWS
        switch (level) {
            case kLogError:     snprintf(buf, 4096, "%s[%d/%d/%d-%d:%02d:%02d:%04d]%s:E:", ANSI_COLOR_RED,     t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, ms, appName); break;
            case kLogWarning:   snprintf(buf, 4096, "%s[%d/%d/%d-%d:%02d:%02d:%04d]%s:W:", ANSI_COLOR_YELLOW,  t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, ms, appName); break;
            case kLogInfo:      snprintf(buf, 4096, "%s[%d/%d/%d-%d:%02d:%02d:%04d]%s:I:", ANSI_COLOR_MAGENTA, t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, ms, appName); break;
            case kLogVerbose:   snprintf(buf, 4096, "%s[%d/%d/%d-%d:%02d:%02d:%04d]%s:V:", ANSI_COLOR_BLUE,    t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, ms, appName); break;
            case kLogDump:      snprintf(buf, 4096, "%s[%d/%d/%d-%d:%02d:%02d:%04d]%s:D:", ANSI_COLOR_CYAN,    t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, ms, appName); break;
            case kLogNone:      break;
            default:            break;
        }
        pos = strlen(buf);
#else
        switch (level) {
            case kLogError:     snprintf(buf, 4096, "[%d/%d/%d-%d:%02d:%02d:%04lld]%s:E:", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, ms.count(), appName); break;
            case kLogWarning:   snprintf(buf, 4096, "[%d/%d/%d-%d:%02d:%02d:%04lld]%s:W:", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, ms.count(), appName); break;
            case kLogInfo:      snprintf(buf, 4096, "[%d/%d/%d-%d:%02d:%02d:%04lld]%s:I:", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, ms.count(), appName); break;
            case kLogVerbose:   snprintf(buf, 4096, "[%d/%d/%d-%d:%02d:%02d:%04lld]%s:V:", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, ms.count(), appName); break;
            case kLogDump:      snprintf(buf, 4096, "[%d/%d/%d-%d:%02d:%02d:%04lld]%s:D:", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, ms.count(), appName); break;
            case kLogNone:      break;
            default:            break;
        }
        pos = strlen(buf);
#endif
    }

    va_list args;
    va_start(args, format);
    vsnprintf(buf + pos, sizeof(buf) - pos, format, args);
    va_end(args);

    pos = strlen(buf);

#ifndef _WINDOWS
    if (!compact)
        snprintf(buf + pos, sizeof(buf) - pos, "%s\n", ANSI_COLOR_RESET);
    else
        snprintf(buf + pos, sizeof(buf) - pos, "%s", ANSI_COLOR_RESET);
    printf("%s", buf);
#else
    if (!compact)
        _snprintf(buf + pos, sizeof(buf) - pos, "\n");
    OutputDebugString(buf);
#endif
    if (s_logFileName != nullptr && s_logFile.is_open())
    {
        s_logFile << buf;
        if (s_logFile.tellp() > MAX_FILE_SIZE)
            Rotate(s_logFileName);
    }
}

} // namespace Log
