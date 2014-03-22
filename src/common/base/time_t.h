#ifndef _TIME_T_H_
#define _TIME_T_H_

#include <time.h>

namespace sails {
namespace common {

class TimeT {
public:
     // returns  the  number  of  bytes
     // (excluding  the  terminating  null byte) placed in the array s.  
     // If the length of the result string (including the terminating null 
     // byte) would exceed  max  bytes,  then strftime() returns 0
    static size_t  time_with_millisecond(char* s, size_t max);
};



} // namespace common
} // namespace sails


#endif /* _TIME_T_H_ */
