#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

static const char* level_strings[6][2] = { {FATAL_COLOR, "[FATAL]: "}, {ERROR_COLOR, "[ERROR]: "}, {WARN_COLOR, "[WARN]: "}, {INFO_COLOR, "[INFO]: "}, {DEBUG_COLOR, "[DEBUG]: "}, {TRACE_COLOR, "[TRACE]: "}};

static int l_string_formated(char** buffer, const char* message, va_list args)
{
    int sucess = 0;

    if(buffer != NULL)
    {
        va_list cpyArgs;
        va_copy(cpyArgs, args); // Cópia de segurança

        // Obtem o número bytes da string formatada
        size_t stringFormatedSize = vsnprintf(NULL, 0, message, cpyArgs);

        if(stringFormatedSize > 0)
        {
            // Aloca memória para a string formatada
            *buffer = (char*)malloc(stringFormatedSize + 1);
            if(*buffer != NULL)
            {
                // Copia para o buffer a string
                vsnprintf((*buffer), stringFormatedSize + 1, message, args);
                sucess = 1;
            }
        }

        va_end(cpyArgs);
    }

    return sucess;
}
void l_log(LogType logType, const char* message, ...)
{
    if(strlen(message) <= 0)
        return;

    va_list args_list;
    va_start(args_list, message);

    char *content = NULL;
    // Setando os argumentos para passar para a thread
    if(l_string_formated(&content, message, args_list))
    {
        time_t t;
        struct tm *time_info;
        
        time(&t);
        time_info = localtime(&t);

        char* time_string = asctime(time_info);
        char hour_minute_second[9];
        strncpy(hour_minute_second, time_string + 11, 8);
        hour_minute_second[8] = '\0';
        printf("%s[%s]%s%s%s\n", level_strings[logType][0], hour_minute_second, level_strings[logType][1], content, RESET);
        
        // Liberando os argumentos alocados para a thread
        free(content);
    }

    va_end(args_list);
}
