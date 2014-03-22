#include <common/log/logging.h>
#include <common/base/time_t.h>
#include <common/base/string.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace sails {
namespace common {
namespace log {

Logger::Logger(Logger::LogLevel level) {
    this->level = level;
    this->save_mode = Logger::SPLIT_NONE;
    memset(filename, '\0', MAX_FILENAME_LEN);
}

Logger::Logger(LogLevel level, char *filename) {
    assert(filename);

    this->level = level;
    this->save_mode = Logger::SPLIT_NONE;
    memset(this->filename, '\0', MAX_FILENAME_LEN);
    strncpy(this->filename, filename, strlen(filename));
}

Logger::Logger(LogLevel level, char *filename, SAVEMODE mode) {
    assert(filename);

    this->level = level;
    this->save_mode = mode;
    memset(this->filename, '\0', MAX_FILENAME_LEN);
    strncpy(this->filename, filename, strlen(filename));
}

void Logger::debug(char *format, ...) {
    if(DEBUG >= level) {
	va_list ap;
	va_start(ap, format);
	this->output(Logger::DEBUG, format, ap);
	va_end(ap);
    }
}

void Logger::info(char *format, ...) {
    if(INFO >= level) {
	va_list ap;
	va_start(ap, format);
	this->output(Logger::INFO, format, ap);
	va_end(ap);
    }
}

void Logger::warn(char *format, ...) {
    if(WARN >= level) {
	va_list ap;
	va_start(ap, format);
	this->output(Logger::WARN, format, ap);
	va_end(ap);
    }
}

void Logger::error(char *format, ...) {
    if(ERROR >= level) {
	va_list ap;
	va_start(ap, format);
	this->output(Logger::ERROR, format, ap);
	va_end(ap);
    }
}

void Logger::output(Logger::LogLevel level, char *format, va_list ap) {
    char msg[1000];
    memset(msg, '\0', 1000);
	
    set_msg_prefix(level, msg);
    vsnprintf(msg+strlen(msg), 900, format, ap);
    msg[strlen(msg)] = '\n';

    char filename[MAX_FILENAME_LEN];
    memset(filename, '\0', MAX_FILENAME_LEN);
    if(this->filename != NULL && strlen(this->filename) > 0) {
	set_filename_by_savemode(filename);
	int write_fd = -1;
	if((write_fd=open(filename, O_WRONLY|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) >= 3) {
	    write(write_fd, msg, strlen(msg));
	    close(write_fd);
	    write_fd = -1;
	}else {
	    char err_msg[20];
	    sprintf(err_msg, "can't open file %s to write\n", filename);
	    write(2, err_msg, strlen(err_msg));
	}
	
    }else {
	write(1, msg, strlen(msg));
    }
}

void Logger::set_filename_by_savemode(char *filename) {
    if(this->filename != NULL && strlen(this->filename) > 0 
       && filename != NULL) {
	size_t index = last_index_of(this->filename,'.');
	strncpy(filename, this->filename, index);
	
	char suffix[10];

	memset(suffix, '\0', 10);
	strcpy(suffix, this->filename+index+1);

	char time[30];
        if(TimeT::time_with_millisecond(time, 30) <= 0) {
	    return;
	}
	
	if(this->save_mode != Logger::SPLIT_NONE) {
	    switch (this->save_mode){
	    case SPLIT_MONTH:
		strncpy(filename+strlen(filename), time, 7);
		break;
	    case SPLIT_DAY:
		strncpy(filename+strlen(filename), time, 10);
		break;
	    case SPLIT_HOUR:
		strncpy(filename+strlen(filename), time, 13);
		break;
	    }
	}

	filename[strlen(filename)] = '.';
	strncpy(filename+strlen(filename), suffix, strlen(suffix));
    }
}

void Logger::set_msg_prefix(Logger::LogLevel level, char *msg) {
    if(TimeT::time_with_millisecond(msg, 1000) <= 0) {
	return;
    }
    msg[strlen(msg)] = ' ';
    if(level == DEBUG) {
	strncpy(msg+strlen(msg), "[debug]", 7);
    }else if(level == INFO) {
	strncpy(msg+strlen(msg), "[info]", 6);
    }else if(level == WARN) {
	strncpy(msg+strlen(msg), "[warn]", 6);
    }else if(level == ERROR) {
	strncpy(msg+strlen(msg), "[error]", 7);
    }
    msg[strlen(msg)] = ':';
}


} // namespace log
} // namespace common
} // namespace sails


