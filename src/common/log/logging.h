#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <stdio.h>
#include <time.h>

namespace sails {
namespace common {
namespace log {

#define MAX_FILENAME_LEN 1000

class Logger {
public:
    enum LogLevel{
	LOG_LEVEL_DEBUG = 1,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR
    };

    enum SAVEMODE{
	SPLIT_NONE,
	SPLIT_MONTH,
	SPLIT_DAY,
	SPLIT_HOUR
    };

    Logger(LogLevel level);
    Logger(LogLevel level, char *filename);
    Logger(LogLevel level, char *filename, SAVEMODE mode);
    
    void debug(char* format, ...);
    void info (char* format, ...);
    void warn (char* format, ...);
    void error(char* format, ...);
private:
    void output(Logger::LogLevel level, char* format, va_list ap);
    void set_msg_prefix(Logger::LogLevel level, char *msg);
    void set_filename_by_savemode(char* filename);
    void check_loginfo();

    LogLevel level;
    char filename[MAX_FILENAME_LEN];    
    SAVEMODE save_mode;
    static char log_config_file[100];
    time_t update_loginfo_time;
};

} // namespace log
} // namespace common
} // namespace sails

#endif /* _LOGGING_H_ */












