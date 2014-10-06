#include <common/base/time_t.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __linux__
#include <sys/time.h>
#include <sys/timeb.h>
#endif
namespace sails {
namespace common {


size_t TimeT::time_str(char*s, size_t max)
{
    int need_len = 20;
    if(max < need_len) {
	return 0;
    }
    memset(s, '\0', need_len);
    time_t temp;
    temp = time(NULL);
    struct tm *t;
    t = localtime(&temp);
    if(strftime(s, max, "%Y-%m-%d %H:%M:%S", t)) {
	return strlen(s);
    }else {
	return 0;
    }
}

size_t TimeT::time_with_millisecond(char* s, size_t max)
{
    int need_len = 24;
    if(max < need_len) {
	return 0;
    }

    memset(s, '\0', need_len);
    time_t temp;
    temp = time(NULL);
    struct tm *t;
    t = localtime(&temp);
    if(strftime(s, max, "%Y-%m-%d %H:%M:%S", t)) {
#ifdef __linux__
	struct timeval t2;
	gettimeofday(&t2, NULL);
	s[strlen(s)] = ' ';
	snprintf(s+strlen(s), 3, "%d", int(t2.tv_usec/1000));
#endif
	return strlen(s);
    }else {
	return 0;
    }
    
}



time_t coverStrToTime(const char* timestr) {
    if (NULL == timestr)
    {
	return 0;
    }
    struct tm tm_;
    int year, month, day, hour, minute,second;
    sscanf(timestr,"%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second);
    tm_.tm_year  = year-1900;
    tm_.tm_mon   = month-1;
    tm_.tm_mday  = day;
    tm_.tm_hour  = hour;
    tm_.tm_min   = minute;
    tm_.tm_sec   = second;
    tm_.tm_isdst = 0;

    time_t t_ = mktime(&tm_);
    return t_;
}


} // namespace common
} // namespace sails







