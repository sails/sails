#include <common/base/util.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

namespace sails {
namespace common {

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
readline(int fd, char *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *ptr;

    ptr = vptr;
    for (n = 1; n < maxlen; n++) {
	if ( (rc = read(fd, &c, 1)) == 1) {
	    *ptr++ = c;
	    if (c == '\n')
		break;
	} else if (rc == 0) {
	    if (n == 1)
		return(0);	/* EOF, no data read */
	    else
		break;		/* EOF, some data was read */
	} else
	    return(-1);	/* error */
    }

    *ptr = 0;
    return(n);
}

} // namespace common  
} // namespace sails
