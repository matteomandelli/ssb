// SSB, v0.01 WIP
// (Logger)

#pragma once

#include "config.hpp"

namespace Log {

enum Level
{
    kLogError,
    kLogWarning,
    kLogInfo,
    kLogVerbose,
    kLogDump,
    kLogNone
};

int Initialise ( Level level, const char* file = nullptr );
int Terminate ( void );

void Log ( Level level, bool compact, const char* appName, const char* format, ... );

} // namespace Log

#if ENABLE_LOG
    //#define LOG                                     Log::Log
    #define LOG_ERROR(_format, ...)         Log::Log(Log::kLogError,    false, APP_NAME, _format, __VA_ARGS__)
    #define LOG_WARNING(_format, ...)       Log::Log(Log::kLogWarning,  false, APP_NAME, _format, __VA_ARGS__)
    #define LOG_INFO(_format, ...)          Log::Log(Log::kLogInfo,     false, APP_NAME, _format, __VA_ARGS__)
    #define LOG_VERBOSE(_format, ...)       Log::Log(Log::kLogVerbose,  false, APP_NAME, _format, __VA_ARGS__)
    #define LOG_VERBOSE_C(_format, ...)     Log::Log(Log::kLogVerbose,  true,  APP_NAME, _format, __VA_ARGS__)
    #define LOG_DUMP(_format, ...)          Log::Log(Log::kLogDump,     false, APP_NAME, _format, __VA_ARGS__)
#else
    //#define LOG                                     sizeof
    #define LOG_ERROR(_format, ...)         sizeof(_format)
    #define LOG_WARNING(_format, ...)       sizeof(_format)
    #define LOG_ERROR(_format, ...)         sizeof(_format)
    #define LOG_INFO(_format, ...)          sizeof(_format)
    #define LOG_VERBOSE(_format, ...)       sizeof(_format)
    #define LOG_VERBOSE_C(_format, ...)     sizeof(_format)
    #define LOG_DUMP(_format, ...)          sizeof(_format)
#endif
