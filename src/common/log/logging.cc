#include <common/log/logging.h>
#include <common/base/time_t.h>
#include <common/base/string.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#ifdef __linux__
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#elif WIN32
#include <io.h>
#include <windows.h>
#endif
#include <fcntl.h>

namespace sails {
namespace common {
namespace log {


char Logger::log_config_file[100] = "./log.conf";

Logger::Logger(Logger::LogLevel level) {
    this->level = level;
    this->save_mode = Logger::SPLIT_NONE;
    memset(filename, '\0', MAX_FILENAME_LEN);
    update_loginfo_time = 0;
}

Logger::Logger(LogLevel level, const char *filename) {
    assert(filename);

    this->level = level;
    this->save_mode = Logger::SPLIT_NONE;
    memset(this->filename, '\0', MAX_FILENAME_LEN);
    update_loginfo_time = 0;

    strncpy(this->filename, filename, strlen(filename));
    set_file_path();
    assert(ensure_directory_exist());
}

Logger::Logger(LogLevel level, const char *filename, SAVEMODE mode) {
    assert(filename);

    this->level = level;
    this->save_mode = mode;
    memset(this->filename, '\0', MAX_FILENAME_LEN);
    update_loginfo_time = 0;

    strncpy(this->filename, filename, strlen(filename));
    set_file_path();
    assert(ensure_directory_exist());
}

void Logger::debug(char *format, ...) {
    check_loginfo();
    if(LOG_LEVEL_DEBUG >= level) {
	va_list ap;
	va_start(ap, format);
	this->output(Logger::LOG_LEVEL_DEBUG, format, ap);
	va_end(ap);
    }
}

void Logger::info(char *format, ...) {
    check_loginfo();
    if(LOG_LEVEL_INFO >= level) {
	va_list ap;
	va_start(ap, format);
	this->output(Logger::LOG_LEVEL_INFO, format, ap);
	va_end(ap);
    }
}

void Logger::warn(char *format, ...) {
    check_loginfo();
    if(LOG_LEVEL_WARN >= level) {
	va_list ap;
	va_start(ap, format);
	this->output(Logger::LOG_LEVEL_WARN, format, ap);
	va_end(ap);
    }
}

void Logger::error(char *format, ...) {
    check_loginfo();
    if(LOG_LEVEL_ERROR >= level) {
	va_list ap;
	va_start(ap, format);
	this->output(Logger::LOG_LEVEL_ERROR, format, ap);
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
#ifdef __linux__
	int write_fd = -1;
	if((write_fd=open(filename, O_WRONLY|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) >= 3) {
	    write(write_fd, msg, strlen(msg));
	    close(write_fd);
	}else {
	    char err_msg[20];
	    sprintf(err_msg, "can't open file %s to write\n", filename);
	    write(2, err_msg, strlen(err_msg));
	}
#else
	FILE* file = fopen(filename, "a");
	if(file != NULL) {
	    fwrite(msg, 1, strlen(msg), file);
	    fclose(file);
	    file = NULL;
	}else {
	    char err_msg[MAX_FILENAME_LEN+30];
	    sprintf(err_msg, "can't open file %s to write\n", filename);
	    fprintf(stderr, err_msg);
	}
#endif
	
    }else {
#ifdef __linux__
	write(1, msg, strlen(msg));
#else
	printf("%s\n", msg);
#endif
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
    if(level == LOG_LEVEL_DEBUG) {
	strncpy(msg+strlen(msg), "[debug]", 7);
    }else if(level == LOG_LEVEL_INFO) {
	strncpy(msg+strlen(msg), "[info]", 6);
    }else if(level == LOG_LEVEL_WARN) {
	strncpy(msg+strlen(msg), "[warn]", 6);
    }else if(level == LOG_LEVEL_ERROR) {
	strncpy(msg+strlen(msg), "[error]", 7);
    }
    msg[strlen(msg)] = ':';
}


void Logger::check_loginfo() {
    time_t current_time = time(NULL);
    if(current_time - update_loginfo_time > 10) {
	FILE* file = fopen(log_config_file, "r");
	if(file != NULL) {
	    char conf[1000];
	    memset(conf, '\0', 1000);
	    if(fread(conf, 1, 1000, file)) {
		if(first_index_of_substr(conf, "LogLevel=DEBUG") >= 0) {
		    this->level = Logger::LOG_LEVEL_DEBUG;
		}else if(first_index_of_substr(conf, "LogLevel=INFO") >=0 ) {
		    this->level = Logger::LOG_LEVEL_INFO;
		}else if(first_index_of_substr(conf, "LogLevel=WARN") >= 0) {
		    this->level = Logger::LOG_LEVEL_WARN;
		}else if(first_index_of_substr(conf, "LogLevel=ERROR") >= 0) {
		    this->level = Logger::LOG_LEVEL_ERROR;
		}
	    }
	    fclose(file);
	}
	update_loginfo_time = current_time;
    }
}

void Logger::set_file_path()
{
    memset(this->path, '\0', MAX_FILENAME_LEN);
    if(strlen(filename) > 0) {
	int index = last_index_of(filename, '/');
	if(index > 0) {
	    strncpy(path, filename, index);
	}else {
	    strncpy(path, "./", 2);
	}
    }
}

bool Logger::ensure_directory_exist()
{
    
#ifdef __linux__
    if(access(path, R_OK|W_OK) != 0) {
	if(mkdir(path, 0766) != 0) {
	    return false;
	}else {
	    return true;
	}
    }else {
	return true;
    }
#elif WIN32
    if(_access(path, 0) != 0) {
	if(_mkdir(path) != 0) {
	    return false;
	}else {
	    return true;
	}
    }else {
	return true;
    }
#endif
}





/////////////////////////////log facotry/////////////////////////////////

std::map<std::string, Logger*> LoggerFactory::log_map;
std::string LoggerFactory::path = "./log";

Logger* LoggerFactory::getLog(std::string log_name)
{
    std::map<std::string, Logger*>::iterator it;
    if((it=log_map.find(log_name)) != log_map.end()) {
	return it->second;
    }else {
	Logger* logger = new Logger(Logger::LOG_LEVEL_INFO, 
				    (path+"/"+log_name+".log").c_str());
	log_map.insert(
	    std::pair<std::string, Logger*>(
		log_name, logger));
	return logger;
    }
}

Logger* LoggerFactory::getLog(std::string log_name, Logger::SAVEMODE save_mode)
{
    std::map<std::string, Logger*>::iterator it;
    if((it=log_map.find(log_name)) != log_map.end()) {
	return it->second;
    }else {
	Logger* logger = new Logger(Logger::LOG_LEVEL_INFO, 
				    (path+"/"+log_name+".log").c_str(), save_mode);
	log_map.insert(
	    std::pair<std::string, Logger*>(log_name,logger));
	return logger;
    }
}


} // namespace log
} // namespace common
} // namespace sails












