#include "sails/net/http_server.h"
#include "sails/base/string.h"
#include "sails/net/mime.h"

class HandleTest {
 public:
  void test1(sails::net::HttpRequest* request,
             sails::net::HttpResponse* response) {
    response->SetBody("call test1", 10);
  }
  void test2(sails::net::HttpRequest* request,
             sails::net::HttpResponse* response) {
    char decode_str[MAX_ELEMENT_SIZE] = {'\0'};
    sails::base::url_decode(request->raw_data->query_string, decode_str,
                            MAX_ELEMENT_SIZE);
    printf("query string:%s\n", decode_str);
    char response_body[1000] = {'\0'};
    snprintf(response_body, sizeof(response_body),
             "call test2, query string:%s",
             decode_str);
    response->SetBody(response_body, strlen(response_body));
  }
  void test3(sails::net::HttpRequest* request,
             sails::net::HttpResponse* response) {
    // 返回json
    response->SetHeader("Content-Type",
                        "application/json");
    std::string data("{\"server\":\"sails\"}");
    response->SetBody(data);
  }
};


bool isRun = true;

void sails_signal_handle(int signo, siginfo_t *, void *) {
  switch (signo) {
    case SIGINT:
      {
        isRun = false;
      }
  }
}


int main(int, char **) {
    // signal kill
    struct sigaction act;
    act.sa_sigaction = sails_signal_handle;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction(SIGINT, &act, NULL) == -1) {
      perror("sigaction error");
      exit(EXIT_FAILURE);
    }

    sails::net::HttpServer server;
    server.Init(8000, 2, 10, 1);
    // server.SetStaticResourcePath("./static/");

    // 请求处理器
    sails::net::HttpServer* httpserver = &server;
    HandleTest test;
    HTTPBIND(httpserver, "/test1", test, HandleTest::test1);
    HTTPBIND(httpserver, "/test2", test, HandleTest::test2);
    HTTPBIND(httpserver, "/test3", test, HandleTest::test3);

    while (isRun) {
      sleep(2);
    }

    server.Stop();

    return 0;
}


