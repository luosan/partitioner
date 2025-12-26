#ifndef CMDLOG_H
#define CMDLOG_H

namespace Shell {

struct cmd_error_exception { };

class CmdLog
{
public:
    static void logInfo(const char *format, ...);
    static void logWarning(const char *format, ...);
    static void logError(const char *format, ...);
    static void logCmdError(const char *format, ...);

    static void addFileAppender(const char *logfile);
private:
    CmdLog(){};
};
}
#endif // CMDLOG_H
