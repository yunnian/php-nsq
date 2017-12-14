#include <php.h>
#include "php_nsq.h"
#include "nsq_lookupd.h"

#include "event2/http.h"
#include "event2/http_struct.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/dns.h"
#include "event2/thread.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/queue.h>
#include <event.h>

typedef struct 
{
  struct event_base * base;
  char *result;
}result;


static zend_class_entry *nsq_lookupd_ce;

static const zend_function_entry nsq_lookupd_functions[] = {
	PHP_FE_END	/* Must be the last line in nsq_functions[] */

};
void lookupd_init(){
    zend_class_entry nsq_lookupd;
    INIT_CLASS_ENTRY(nsq_lookupd,"NsqLookupd",nsq_lookupd_functions);
    //nsq_lookupd_ce = zend_register_internal_class_ex(&nsq_lookupd,NULL,NULL TSRMLS_CC);
    nsq_lookupd_ce = zend_register_internal_class(&nsq_lookupd TSRMLS_CC);
    zend_declare_property_null(nsq_lookupd_ce,ZEND_STRL("address"),ZEND_ACC_PUBLIC TSRMLS_CC);
}

PHP_METHOD(NsqLookupd, __construct){
    zval *self;
    zval *address;
    self = getThis();

	ZEND_PARSE_PARAMETERS_START(1,1)
        Z_PARAM_ZVAL(address)
	ZEND_PARSE_PARAMETERS_END();
    zend_update_property(Z_OBJCE_P(self),self,ZEND_STRL("address"),address TSRMLS_CC);
}

void FinshCallback(struct evhttp_request* remote_rsp, void* arg)
{
    //printf("dddddd%s",arg);
    result * re = arg;
    const int code = remote_rsp  ?  evhttp_request_get_response_code (remote_rsp)  :  0;
    printf("code:%d",code);
	struct evbuffer *buf = evhttp_request_get_input_buffer(remote_rsp);
    evbuffer_add (buf, "", 1);    /* NUL-terminate the buffer */
    char *payload = (char *) evbuffer_pullup(buf, -1);
    re->result = strdup(payload);
    printf("ssss:%s",(re->result));
    event_base_loopbreak(re->base);
    //event_base_loopexit((struct event_base*)arg, NULL);
} 


void RequestErrorCallback(enum evhttp_request_error error, void* arg)
{
    fprintf(stderr, "request failed\n");
    event_base_loopexit((struct event_base*)arg, NULL);
}

void ConnectionCloseCallback(struct evhttp_connection* connection, void* arg)
{
    fprintf(stderr, "remote connection closed\n");
    event_base_loopexit((struct event_base*)arg, NULL);
}

char* lookup(char *host, char* topic){
    char * url = emalloc(sizeof(host) + sizeof(topic) + 20);
    sprintf(url, "%s%s", host, "lookupd", "\n");
    if(strstr(url,"http://")){
        sprintf(url, "%s%s%s", host, "/lookup?topic=", topic);
    }else{
        sprintf(url, "%s%s%s%s", "http://", host, "/lookup?topic=", topic); 
    }
    char *data =  request(url);
    printf("data:%s",data);
	efree(url);
    return data;

}

char* request(char* url)
{
    printf("url:%s",url);
    char * msg ;
    struct evhttp_uri* uri = evhttp_uri_parse(url);
    if (!uri)
    {
        fprintf(stderr, "parse url failed!\n");
        msg =  "{\"message\":\"parse url failed!\"}";
        return msg;
    }

    struct event_base* base = event_base_new();
    if (!base)
    {
        fprintf(stderr, "create event base failed!\n");
        msg =  "{\"message\":\"create event base failed!\"}";
        return msg;
    }

    struct evdns_base* dnsbase = evdns_base_new(base, 1);
    if (!dnsbase)
    {
        fprintf(stderr, "create dns base failed!\n");
        msg =  "{\"message\":\"create dns base failed!\"}";
        return msg;
    }
    assert(dnsbase);

    result * re ;
    re->base = base;
    struct evhttp_request* request = evhttp_request_new(FinshCallback, re);
    evhttp_request_set_error_cb(request, RequestErrorCallback);

    const char* host = evhttp_uri_get_host(uri);
    if (!host)
    {
        fprintf(stderr, "parse host failed!\n");
        msg = "{\"message\":\"stderr, parse host failed!\"}";
        return msg;
    }

	
    int port = evhttp_uri_get_port(uri);
    if (port < 0) port = 80;

    const char* request_url = url;
    const char* path = evhttp_uri_get_path(uri);
    if (path == NULL || strlen(path) == 0)
    {
        request_url = "/";
    }

    struct evhttp_connection* connection =  evhttp_connection_base_new(base, dnsbase, host, port);
    if (!connection)
    {
        fprintf(stderr, "create evhttp connection failed!\n");
        msg =  "{\"message\":\"create evhttp connection failed!\"}";
        return msg;
    }

    evhttp_connection_set_closecb(connection, ConnectionCloseCallback, base);

    evhttp_add_header(evhttp_request_get_output_headers(request), "Host", host);
    evhttp_make_request(connection, request, EVHTTP_REQ_GET, request_url);

    event_base_dispatch(base);
    return re->result;
}
