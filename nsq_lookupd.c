/*
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2017 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Zhenyu wu      <wuzhenyu@kuangjue.com>                       |
  +----------------------------------------------------------------------+
*/

#include <php.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "ext/standard/php_var.h"
#include "nsq_lookupd.h"
#include "common.h"

#include "event2/http.h"
#include "event2/http_struct.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/dns.h"
#include "event2/thread.h"

#include <event.h>

extern void error_handling(char *message);

typedef struct {
    struct event_base *base;
    char *result;
} result;


ZEND_BEGIN_ARG_INFO_EX(ctor, 0, 0, 1)
    ZEND_ARG_INFO(0, address)
ZEND_END_ARG_INFO()

static PHP_METHOD(NsqLookupd, __construct);

zend_class_entry *nsq_lookupd_ce;

static const zend_function_entry nsq_lookupd_functions[] = {
    PHP_ME(NsqLookupd, __construct, ctor, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_FE_END    /* Must be the last line in nsq_functions[] */

};

void lookupd_init() {
    zend_class_entry nsq_lookupd;
    INIT_CLASS_ENTRY(nsq_lookupd, "NsqLookupd", nsq_lookupd_functions);
    nsq_lookupd_ce = zend_register_internal_class(&nsq_lookupd);
    zend_declare_property_null(nsq_lookupd_ce, ZEND_STRL("address"), ZEND_ACC_PUBLIC);
}

PHP_METHOD (NsqLookupd, __construct) {
    zval *self;
    zval *address;
    self = getThis();
    ZEND_PARSE_PARAMETERS_START(1, 1)
            Z_PARAM_ZVAL(address)
    ZEND_PARSE_PARAMETERS_END();
    zend_update_property(Z_OBJCE_P(self),  NSQ_COMPAT_OBJ_P(self), ZEND_STRL("address"), address);
}

void FinshCallback(struct evhttp_request *remote_rsp, void *arg) {
    result *re = arg;
    const int code = remote_rsp ? evhttp_request_get_response_code(remote_rsp) : 0;
    struct evbuffer *buf = evhttp_request_get_input_buffer(remote_rsp);
    evbuffer_add(buf, "", 1);    /* NUL-terminate the buffer */
    char *payload = (char *) evbuffer_pullup(buf, -1);
    re->result = strdup(payload);
    event_base_loopbreak(re->base);
    //event_base_loopexit((struct event_base*)arg, NULL);
}


void RequestErrorCallback(enum evhttp_request_error *error, void *arg) {
    fprintf(stderr, "request failed\n");
    event_base_loopexit((struct event_base *) arg, NULL);
}

void ConnectionCloseCallback(struct evhttp_connection *connection, void *arg) {
    fprintf(stderr, "remote connection closed\n");
    event_base_loopexit((struct event_base *) arg, NULL);
}

char *lookup(char *host, char *topic) {
    char *url = emalloc(sizeof(host) + sizeof(topic) + 128);
    if (strstr(host, "http://")) {
        sprintf(url, "%s%s%s", host, "/lookup?topic=", topic);
    } else {
        sprintf(url, "%s%s%s%s", "http://", host, "/lookup?topic=", topic);
    }
    char *data = request(url);
    efree(url);
    return data;

}

char *request(char *url) {
    char *msg ;
    struct evhttp_uri *uri = evhttp_uri_parse(url);
    if (!uri) {
        fprintf(stderr, "parse url failed!\n");
        msg = "{\"message\":\"parse url failed!\"}";
        return msg;
    }

    struct event_base *base = event_base_new();
    if (!base) {
        fprintf(stderr, "create event base failed!\n");
        msg = "{\"message\":\"create event base failed!\"}";
        return msg;
    }

    struct evdns_base *dnsbase = evdns_base_new(base, 1);
    if (!dnsbase) {
        fprintf(stderr, "create dns base failed!\n");
        msg = "{\"message\":\"create dns base failed!\"}";
        return msg;
    }
    assert(dnsbase);

    result * re  = (result * ) emalloc ( sizeof(result) );
    re->base = base;

    struct evhttp_request *request = evhttp_request_new(FinshCallback, re);
    //evhttp_request_set_error_cb(request, RequestErrorCallback);

    const char *host = evhttp_uri_get_host(uri);
    if (!host) {
        fprintf(stderr, "parse host failed!\n");
        msg = "{\"message\":\"stderr, parse host failed!\"}";
        return msg;
    }


    int port = evhttp_uri_get_port(uri);
    if (port < 0) port = 80;

    const char *request_url = url;
    const char *path = evhttp_uri_get_path(uri);
    if (path == NULL || strlen(path) == 0) {
        request_url = "/";
    }

    struct evhttp_connection *connection = evhttp_connection_base_new(base, dnsbase, host, port);
    if (!connection) {
        fprintf(stderr, "create evhttp connection failed!\n");
        msg = "{\"message\":\"create evhttp connection failed!\"}";
        return msg;
    }

    evhttp_connection_set_closecb(connection, ConnectionCloseCallback, base);

    evhttp_add_header(evhttp_request_get_output_headers(request), "Host", host);
    evhttp_make_request(connection, request, EVHTTP_REQ_GET, request_url);

    event_base_dispatch(base);
    return re->result;
}
