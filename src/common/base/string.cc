#include <common/base/string.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

namespace sails {
namespace common {


size_t
strlncat(char *dst, size_t len, const char *src, size_t n)
{
  size_t slen;
  size_t dlen;
  size_t rlen;
  size_t ncpy;

  slen = strnlen(src, n);
  dlen = strnlen(dst, len);

  if (dlen < len) {
    rlen = len - dlen;
    ncpy = slen < rlen ? slen : (rlen - 1);
    memcpy(dst + dlen, src, ncpy);
    dst[dlen + ncpy] = '\0';
    
  }

  assert(len > slen + dlen);
  return slen + dlen;
}

size_t
strlcat(char *dst, const char *src, size_t len)
{
  return strlncat(dst, len, src, (size_t) -1);
}

size_t
strlncpy(char *dst, size_t len, const char *src, size_t n)
{
  size_t slen;
  size_t ncpy;

  slen = strnlen(src, n);

  if (len > 0) {
    ncpy = slen < len ? slen : (len - 1);
    memcpy(dst, src, ncpy);
    dst[ncpy] = '\0';
  }

  assert(len > slen);
  return slen;
}

size_t
strlcpy(char *dst, const char *src, size_t len)
{
  return strlncpy(dst, len, src, (size_t) -1);
}

size_t
last_index_of(const char* src, char c)
{
    size_t index = -1;
    int len = 0;
    if ((len=strlen(src)) > 0) {
	for (int i = len-1; i >= 0; i-- ) {
	    if(src[i] == c) {
		index = i;
		break;
	    }
	}
    }

    return index;
}

size_t
first_index_of(const char* src, char c) 
{
    size_t index = -1;
    int len = 0;
    if ((len=strlen(src)) > 0) {
	for (int i = 0; i < len; i++ ) {
	    if(src[i] == c) {
		index = i;
		break;
	    }
	}
    }
    return index;
}

} // namespace common
} // namespace sails
