#ifndef _HTTP_CONNECTOR_H_
#define _HTTP_CONNECTOR_H_
#include <list>
#include <http_parser.h>
#include <common/net/http.h>
#include <common/net/connector.h>

namespace sails {
namespace common {
namespace net {

class HttpConnector : public Connector {

public:
    HttpConnector(int connect_fd);
    HttpConnector(); // and then call connect
    ~HttpConnector();

    void httpparser();
    HttpRequest* get_next_httprequest();
    HttpResponse* get_next_httpresponse();

private:
    // http_parser call_back
    static int request_url_cb(http_parser *p, const char *buf, size_t len);
    static int status_complete_cb(http_parser *p);
    static int header_field_cb(http_parser *p, const char *buf, size_t len);
    static int header_value_cb(http_parser *p, const char *buf, size_t len);
    static void check_body_is_final(const http_parser *p);
    static int body_cb (http_parser *p, const char *buf, size_t len);
    int count_body_cb (http_parser *p, const char *buf, size_t len);
    static int message_begin_cb(http_parser *p);
    static int headers_complete_cb(http_parser *p);
    static int message_complete_cb(http_parser *p);

private:
    
    std::list<HttpRequest *> req_list;
    std::list<HttpResponse *> rep_list;
    
    http_parser parser;
    struct http_message *message;
    static struct http_parser_settings settings;
    void handle_request();
    void parser_url(char *url);

    void push_request_list(HttpRequest *req);
    void push_response_list(HttpResponse *rep);
};

} // namespace net
} // namespace common
} // namespace sails

#endif /* _HTTP_CONNECTOR_H_ */

