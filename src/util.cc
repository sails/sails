#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

namespace sails {

void setnonblocking(int fd)
{
     int opts;
     opts=fcntl(fd,F_GETFL);
     if(opts<0)
     {
          perror("fcntl(fd,GETFL)");
          exit(EXIT_FAILURE);
     }
     opts = opts|O_NONBLOCK;
     if(fcntl(fd,F_SETFL,opts)<0)
     {
          perror("fcntl(fd,SETFL,opts)");
          exit(EXIT_FAILURE);
     }  
}

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
	
}
