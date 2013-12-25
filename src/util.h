#ifndef _UTIL_H_
#define _UTIL_H_

#include <cstddef>
#include <string.h>
#include <assert.h>

namespace sails {

// cat n byte of src to dst, len stand for the maxlen of dst
size_t strlncat(char *dst, size_t len, const char *src, size_t n);

size_t strlcat(char *dst, const char *src, size_t len);

size_t strlncpy(char *dst, size_t len, const char *src, size_t n);

size_t strlcpy(char *dst, const char *src, size_t len);

}

#endif /* _UTIL_H_ */
















