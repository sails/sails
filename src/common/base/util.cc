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

} // namespace common  
} // namespace sails
