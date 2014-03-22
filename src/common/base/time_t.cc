#include <common/base/time_t.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/timeb.h>

namespace sails {
namespace common {

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
	struct timeval t2;
	gettimeofday(&t2, NULL);
	s[strlen(s)] = ' ';
	snprintf(s+strlen(s), 3, "%d", int(t2.tv_usec/1000));
	return strlen(s);
    }else {
	return 0;
    }
    
}

} // namespace common
} // namespace sails















