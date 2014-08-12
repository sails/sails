#include <common/net/server.h>
#include <common/net/http.h>
#include <http_parser.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <common/base/string.h>

namespace sails {



///////////////////////////////////http parser///////////////////////////

struct ParserStruct {
    http_parser* parser;
    common::net::http_message* m;
};


int request_url_cb(http_parser *p, const char *buf, size_t len);
int status_complete_cb(http_parser *p);
int header_field_cb(http_parser *p, const char *buf, size_t len);
int header_value_cb(http_parser *p, const char *buf, size_t len);
void check_body_is_final(const http_parser *p);
int body_cb (http_parser *p, const char *buf, size_t len);
int message_begin_cb(http_parser *p);
int headers_complete_cb(http_parser *p);
int message_complete_cb(http_parser *p);
void parser_url(common::net::http_message *m);


struct http_parser_settings settings = {
on_message_begin : message_begin_cb,
on_url : request_url_cb,
on_status_complete : status_complete_cb,
on_header_field : header_field_cb,
on_header_value : header_value_cb,
on_headers_complete : headers_complete_cb,
on_body : body_cb,
on_message_complete : message_complete_cb
};

std::shared_ptr<common::net::Connector<common::net::HttpRequest>> getConnectorFromParser(const http_parser *p) {
    if(p->data == NULL) {
	printf("p->data == null\n");
	return NULL;
    }
    common::net::SharedPtrAdapter<common::net::Connector<common::net::HttpRequest>>* connectorAdapter = (common::net::SharedPtrAdapter<common::net::Connector<common::net::HttpRequest>>*)(p->data);
    return connectorAdapter->ptr;
}

common::net::http_message* getParseringMsgFromPasrser(const http_parser *p) {
    std::shared_ptr<common::net::Connector<common::net::HttpRequest>> connector = getConnectorFromParser(p);
    if (connector != NULL) {
	ParserStruct *parserStruct = (ParserStruct*)connector->data;
	if (parserStruct != NULL) {
	    return parserStruct->m;
	}
    }
    return NULL;
}


int request_url_cb (http_parser *p, const char *buf, size_t len)
{
    if(p->data == NULL) {
	printf("p->data == null\n");
    }

    common::net::http_message *m = getParseringMsgFromPasrser(p);

    common::strlncat(m->request_url,
	     sizeof(m->request_url),
	     buf,
	     len);

    return 0;
}

int status_complete_cb (http_parser *p) {
    std::shared_ptr<common::net::Connector<common::net::HttpRequest>> connector = getConnectorFromParser(p);
    if(connector != NULL) {
	return 0;	  
    }
    return -1;
}

int header_field_cb (http_parser *p, const char *buf, size_t len)
{
    common::net::http_message *m = getParseringMsgFromPasrser(p);

    if (m->last_header_element != common::net::FIELD)
	m->num_headers++;
	
    common::strlncat(m->headers[m->num_headers-1][0],
	     sizeof(m->headers[m->num_headers-1][0]),
	     buf,
	     len);

    m->last_header_element = common::net::FIELD;

    return 0;
}

int header_value_cb (http_parser *p, const char *buf, size_t len)
{
    common::net::http_message *m = getParseringMsgFromPasrser(p);

    common::strlncat(m->headers[m->num_headers-1][1],
	     sizeof(m->headers[m->num_headers-1][1]),
	     buf,
	     len);
    m->last_header_element = common::net::VALUE;

    return 0;
}

void check_body_is_final (const http_parser *p)
{
    common::net::http_message *m = getParseringMsgFromPasrser(p);

    if (m->body_is_final) {
	fprintf(stderr, "\n\n *** Error http_body_is_final() should return 1 on last on_body callback call "
		"but it doesn't! ***\n\n");
	assert(0);
	abort();
    }

    m->body_is_final = http_body_is_final(p);
}

int body_cb (http_parser *p, const char *buf, size_t len)
{
    common::net::http_message *m = getParseringMsgFromPasrser(p);

    common::strlncat(m->body,
	     sizeof(m->body),
	     buf,
	     len);
    m->body_size += len;
    check_body_is_final(p);

    return 0;
}

int count_body_cb (http_parser *p, const char *buf, size_t len)
{
    common::net::http_message *m = getParseringMsgFromPasrser(p);

    assert(buf);
    m->body_size += len;
    check_body_is_final(p);

    return 0;
}

int message_begin_cb (http_parser *p)
{
    common::net::http_message *m = getParseringMsgFromPasrser(p);
    m->message_begin_cb_called = TRUE;
    return 0;
}

int headers_complete_cb (http_parser *p)
{
    common::net::http_message *m = getParseringMsgFromPasrser(p);

    m->method = p->method;
    m->status_code = p->status_code;
    m->http_major = p->http_major;
    m->http_minor = p->http_minor;
    m->headers_complete_cb_called = TRUE;
    m->should_keep_alive = http_should_keep_alive(p);

    parser_url(m);

    return 0;
}

int message_complete_cb (http_parser *p)
{
    std::shared_ptr<common::net::Connector<common::net::HttpRequest>> connector = getConnectorFromParser(p);

    common::net::http_message *m = getParseringMsgFromPasrser(p);

    if (m->should_keep_alive != http_should_keep_alive(p))
    {
	fprintf(stderr, "\n\n *** Error http_should_keep_alive() should have same "
		"value in both on_message_complete and on_headers_complete "
		"but it doesn't! ***\n\n");
	assert(0);
	abort();
    }

    if (m->body_size &&
	http_body_is_final(p) &&
	!m->body_is_final)
    {
	fprintf(stderr, "\n\n *** Error http_body_is_final() should return 1 "
		"on last on_body callback call "
		"but it doesn't! ***\n\n");
	assert(0);
	abort();
    }
	
    m->message_complete_cb_called = TRUE;
    if(m->body_size == 0) {
	m->message_complete_on_eof = TRUE;
    }
    common::net::HttpRequest* request = new common::net::HttpRequest(m);
    connector->push_recv_list(request);
	
    common::net::http_message* message = (common::net::http_message*)malloc(
	sizeof(common::net::http_message));
    assert(message != NULL);

    http_message_init(message);
    if (connector != NULL) {
	ParserStruct *parserStruct = (ParserStruct*)connector->data;
	if (parserStruct != NULL) {
	    parserStruct->m = message;
	}
    }


    return 0;
}


void parser_url(common::net::http_message *m)
{
    struct http_parser_url u;
    int url_result = 0;

    char url[200];
    memset(url, 0, 200);
    strncat(url, "http://", strlen("http://"));
    for(int i = 0; i < m->num_headers; i++) {
	if(strcmp("Host", m->headers[i][0]) == 0) {
	    strncat(url, m->headers[i][1], strlen(m->headers[i][1]));
	    break;
	}
    }
    strncat(url, m->request_url, strlen(m->request_url));

    if((url_result = http_parser_parse_url(url, strlen(url), 0, &u)) == 0)
    {
	if(u.field_set & (1 << UF_PORT)) {
	    m->port = u.port;
	}else {
	    m->port = 80;
	}
	if(m->host) {
	    free(m->host);
	}
	if(u.field_set & (1 << UF_HOST)) {
	    m->host = (char*)malloc(u.field_data[UF_HOST].len+1);
	    strncpy(m->host, url+u.field_data[UF_HOST].off, u.field_data[UF_HOST].len);  
	    m->host[u.field_data[UF_HOST].len] = 0;  
	}
	memset(m->request_path, 0, strlen(m->request_path));
	if(u.field_set & (1 << UF_PATH))  
	{  
	    strncpy(m->request_path, url+u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);  
	    m->request_path[u.field_data[UF_PATH].len] = 0;  
	}

    }else {
	printf("url parser error:%d\n", url_result);
    }
	
}

void printf_http_message(common::net::http_message *message) {
    
    printf("message:\n");
    printf("request_url:%s\n", message->request_url);
    printf("port:%d\n", message->port);
    printf("request_path:%s\n", message->request_path);
    printf("host:%s\n", message->host);
    printf("should_keep_alive:%d\n", message->should_keep_alive);

    for(int i = 0;i < message->num_headers; i++) {
	printf("header %d:%s:%s\n", i, message->headers[i][0], 
	       message->headers[i][1]);
    }
    printf("body:%s\n", message->body);
}

///////////////////////////////////http parser///////////////////////////















typedef sails::common::net::Server<common::net::HttpRequest> HttpServer;
HttpServer server;


  
void accept_after_cb(std::shared_ptr<common::net::Connector<common::net::HttpRequest>> connector, struct sockaddr_in &local) {
    http_parser* httpparser = (http_parser*)malloc(sizeof(http_parser));
    http_parser_init(httpparser, ::HTTP_BOTH);

    common::net::SharedPtrAdapter<common::net::Connector<common::net::HttpRequest>>* connectorAdapter = new common::net::SharedPtrAdapter<common::net::Connector<common::net::HttpRequest>>();
    connectorAdapter->ptr = connector;
    httpparser->data = connectorAdapter;

    common::net::http_message* message = (common::net::http_message*)malloc(
	sizeof(common::net::http_message));
    assert(message != NULL);
    http_message_init(message);
    
    struct ParserStruct* parserStruct = (struct ParserStruct*)malloc(sizeof(struct ParserStruct));
    memset(parserStruct, 0, sizeof(struct ParserStruct));
    parserStruct->parser = httpparser;
    parserStruct->m = message;

    connector->data = parserStruct;

}

common::net::HttpRequest* parser(common::net::Connector<common::net::HttpRequest> *connector) {

    if (connector->readable() <= 0) {
	return NULL;
    }
    int readable = connector->readable();
    struct ParserStruct* parserStruct = (struct ParserStruct*)connector->data;
    if (parserStruct != NULL) {
	http_parser* parser = parserStruct->parser;

	size_t nparsed = http_parser_execute(parser, &settings, 
					     connector->peek(), readable);
	connector->retrieve(nparsed);
    }
    return NULL;
}


void handle_fun(std::shared_ptr<common::net::Connector<common::net::HttpRequest>> connector, common::net::HttpRequest *request) {
    int connfd = connector->get_connector_fd();

    common::net::HttpResponse *response = new common::net::HttpResponse();
    response->connfd = connfd;


    std::string request_path = std::string(request->raw_data->request_path);
    std::string real_path = "./static"+request_path;
    if (real_path == "./static/") {
	real_path+="index.html";
    }
    std::cout << "real_path:" << real_path << std::endl;
    std::string content;
    std::string line;
    std::ifstream file(real_path);
    if (file.is_open())
    {
	while ( getline (file,line) )
	{
	    content+=line;
	}
	content+="\r\n";
	response->set_body(content.c_str());
	
	file.close();
    }
    std::cout << "content:" << content << std::endl;

    response->to_str();
    
    int n = write(response->connfd, response->get_raw(), 
		      strlen(response->get_raw()));
    if(request->raw_data->should_keep_alive != 1) {
	connector->close();
    }else {
	
    }
    
    delete response;
    
}

void connector_close_cb(common::net::Connector<common::net::HttpRequest> *connector) {
    struct ParserStruct* parserStruct = (struct ParserStruct*)connector->data;
    printf("connector close cb\n");
    if (parserStruct != NULL) {

	http_parser* parser = parserStruct->parser;
	common::net::http_message* msg = parserStruct->m;
	if (parser != NULL) {
	    common::net::SharedPtrAdapter<common::net::Connector<common::net::HttpRequest>>* connectorAdapter = (common::net::SharedPtrAdapter<common::net::Connector<common::net::HttpRequest>>*)(parser->data);
	    if (connectorAdapter != NULL) {
		delete connectorAdapter;
	    }
	    free(parser);
	}
	if (msg != NULL) {
	    free(msg);
	}
        free(parserStruct);
	connector->data = NULL;
    }
}


} // namespace sails


void sails_signal_handle(int signo, siginfo_t *info, void *ext) {
    switch(signo) {
	case SIGINT:
	{
	    printf("stop\n");
	    sails::server.stop();
	    break;
	}
    }
}

int main(int argc, char *argv[])
{
     // signal kill
    struct sigaction act;
    act.sa_sigaction = sails_signal_handle;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if(sigaction(SIGINT, &act, NULL) == -1) {
	perror("sigaction error");
	exit(EXIT_FAILURE);
    }
    
    sails::server.init(8000, 10, 4, 1000, 2); //default listen 8000 port
    sails::server.set_accept_after_cb(sails::accept_after_cb);
    sails::server.set_parser_cb(sails::parser);
    sails::server.set_handle_cb(sails::handle_fun);
    sails::server.set_connector_close_cb(sails::connector_close_cb);
    sails::server.start();

    return 0;
}

