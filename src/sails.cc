#include "sails.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ev.h>
#include "connection.h"

namespace sails {

void sails_init() {
	
}

void init_config(int argc, char *argv[]) {
	
}

} //namespace sails



int main(int argc, char *argv[])
{
	int  listenfd = 0;
	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("create sock error !");
		return -1;
	}
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(8000);

	 // ignore SIGPIPE
	struct sigaction on_sigpipe;
	on_sigpipe.sa_handler = SIG_IGN;
	sigemptyset(&on_sigpipe.sa_mask);
	sigaction(SIGPIPE, &on_sigpipe, NULL);

	bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	fcntl(listenfd, F_SETFL, O_NONBLOCK);

	printf("listen\n");
	if(listen(listenfd, 10) >= 0) {
		struct ev_loop *loop = ev_default_loop(0);
		//new listen watcher
		struct ev_io accept_w;
		ev_io_init(&accept_w, sails::Connection::accept_cb, listenfd, EV_READ);
		ev_io_start(loop, &accept_w);
		ev_loop(loop, 0);
	}
	return 0;
}
