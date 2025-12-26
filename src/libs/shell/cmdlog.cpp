#include "cmdlog.h"
#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Formatters/MessageOnlyFormatter.h>

using namespace Shell;

void CmdLog::logInfo(const char *format, ...)
{
    char* str = NULL;
    va_list ap;
    va_start(ap, format);
    int len = vasprintf(&str, format, ap);
    static_cast<void>(len);
    va_end(ap);

    PLOGI << str;

    free(str);

}

void CmdLog::logWarning(const char *format, ...)
{
    char* str = NULL;
    va_list ap;
    va_start(ap, format);
    int len = vasprintf(&str, format, ap);
    static_cast<void>(len);
    va_end(ap);

    PLOGW << str;

    free(str);

}

void CmdLog::logError(const char *format, ...)
{
    char* str = NULL;
    va_list ap;
    va_start(ap, format);
    int len = vasprintf(&str, format, ap);
    static_cast<void>(len);
    va_end(ap);

    PLOGE << str;

    free(str);
}

void CmdLog::logCmdError(const char *format, ...)
{

    char* str = NULL;
    va_list ap;
    va_start(ap, format);
    int len = vasprintf(&str, format, ap);
    static_cast<void>(len);
    va_end(ap);
    PLOGE << str;
    free(str);
    throw cmd_error_exception();
}

void CmdLog::addFileAppender(const char *logfile)
{
    static plog::RollingFileAppender<plog::MessageOnlyFormatter> fileAppender(logfile);
    plog::get()->addAppender(&fileAppender);
}
