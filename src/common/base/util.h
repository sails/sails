#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdio.h>

namespace sails {
namespace common {

void setnonblocking(int fd);

size_t
readline(int fd, void *vptr, size_t maxlen);

} // namespace common  
} // namespace sails

#endif /* _UTIL_H_ */
