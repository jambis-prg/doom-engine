#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#include "typedefs.h"

typedef enum
{
    LOG_FATAL,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
    LOG_TRACE
} LogType;

void l_log(LogType logType, const char* message, ...);

// LOG COLORS
#define FATAL_COLOR "\e[1;90m\e[41m"    // Bold Black Text, Red Background
#define ERROR_COLOR "\e[1;91m"          // Bold Red Text
#define WARN_COLOR  "\e[0;33m"          // Yellow Color
#define INFO_COLOR  "\e[0;32m"          // Green Color
#define DEBUG_COLOR "\e[0;34m"          // Blue Color
#define TRACE_COLOR "\e[0;37m"          // White Color
#define RESET       "\e[0m"             // Reset All

#ifdef _DEBUG
    #define DOOM_LOG_FATAL(MESSAGE, ...)     l_log(LOG_FATAL, MESSAGE, ##__VA_ARGS__)
    #define DOOM_LOG_ERROR(MESSAGE, ...)     l_log(LOG_ERROR, MESSAGE, ##__VA_ARGS__)
    #define DOOM_LOG_WARN(MESSAGE, ...)      l_log(LOG_WARN, MESSAGE, ##__VA_ARGS__)
    #define DOOM_LOG_INFO(MESSAGE, ...)      l_log(LOG_INFO, MESSAGE, ##__VA_ARGS__)
    #define DOOM_LOG_DEBUG(MESSAGE, ...)     l_log(LOG_DEBUG, MESSAGE, ##__VA_ARGS__)
    #define DOOM_LOG_TRACE(MESSAGE, ...)     l_log(LOG_TRACE, MESSAGE, ##__VA_ARGS__)
#else
    #define DOOM_LOG_FATAL(MESSAGE, ...)
    #define DOOM_LOG_ERROR(MESSAGE, ...)
    #define DOOM_LOG_WARN(MESSAGE, ...)
    #define DOOM_LOG_INFO(MESSAGE, ...)
    #define DOOM_LOG_DEBUG(MESSAGE, ...)
    #define DOOM_LOG_TRACE(MESSAGE, ...)                 
#endif

#endif