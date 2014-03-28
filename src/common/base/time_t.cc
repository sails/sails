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
    if(strftime(s, max, "%F %T", t)) {
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
    if(strftime(s, max, "%F %T", t)) {
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

} // namespace common
} // namespace sails







