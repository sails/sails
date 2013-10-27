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
#include <http_parser.h>
#include "http.h"

namespace sails {

	void sails_init() {
		
	}
	void init_config(int argc, char *argv[]) {
		
	}

	void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
		if(EV_ERROR & revents) {
			printf("accept error!\n");
			return;
		}
		struct sockaddr_in clientaddr;
		socklen_t len = sizeof(struct sockaddr);
		int listenfd = watcher->fd;
		struct ev_io *recv_w = (struct ev_io *)malloc(sizeof(struct ev_io));
		int connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &len);
		fcntl(connfd, F_SETFL, O_NONBLOCK);
		
		ev_io_init(recv_w, recv_cb, connfd, EV_READ);
		ev_io_start(loop, recv_w);

	}
	void recv_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
		if(EV_ERROR & revents) {
			printf("recv error!\n");
			return;
		}
		if(!EV_READ) {
			printf("not readable \n");
		}
		printf("start read\n");
 		int connfd = watcher->fd;
		int len = 80 * 1024;
		char buf[len];
		int n = 0;
		n = read(connfd, buf, len);
		
		if(n == -1) {
			//error
			ev_io_stop(loop, watcher);
			free(watcher);
		}
		if(n == 0) {
			return;
		}
		if(n > 0) {
			printf("%s", buf);
			buf[n] = '\0';
			HttpConnection connection;
			http_parser *parser = connection.parser_http(buf);
			if(parser != NULL){
				printf("http_major:%d\n", parser->http_major);
				printf("http_method:%d\n", parser->method);
			}
		}

		printf("recv n:%d\n", n);
		printf("end recv\n");
	}

}



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
		ev_io_init(&accept_w, sails::accept_cb, listenfd, EV_READ);
		ev_io_start(loop, &accept_w);
		ev_loop(loop, 0);
	}
	return 0;
}
