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
  | Author: Zhenyu Wu      <wuzhenyu@kuangjue.com>                       |
  +----------------------------------------------------------------------+
*/

#include "php.h"
#include <arpa/inet.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include "sub.h"
#include "command.h"
#include "common.h"
#include "ext/standard/php_var.h"

extern zend_class_entry *nsq_message_ce;

extern void error_handlings(char *message);

void conn_writecb(struct bufferevent *, void *);

void readcb(struct bufferevent *, void *msg);

void conn_eventcb(struct bufferevent *, short, void *);

extern int le_bufferevent;

int subscribe(NSQArg *arg) {
    struct sockaddr_in srv;
    struct hostent *he;
    memset(&srv, 0, sizeof(srv));
    int retry_num = 1;

    if (check_ipaddr(arg->host)) {
        srv.sin_addr.s_addr = inet_addr(arg->host);
    } else {
        /* resolve hostname */
        if ((he = gethostbyname(arg->host)) == NULL) {
            exit(1); /* error */
        }
        /* copy the network address to sockaddr_in structure */
        memcpy(&srv.sin_addr, he->h_addr_list[0], he->h_length);
    }
    srv.sin_family = AF_INET;
    srv.sin_port = htons(atoi(arg->port));
    struct event_base *base = event_base_new();
    if (!base) {
        printf("Could not initialize libevent\n");
        return 1;
    }

    struct bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    arg->bev_res =  zend_register_resource(bev, le_bufferevent);

    //监听终端输入事件 暂时用不上 
    //struct event* ev_cmd = event_new(base, STDIN_FILENO,  EV_READ | EV_PERSIST,  cmd_msg_cb, (void*)bev)

    bufferevent_setcb(bev, readcb, NULL, conn_eventcb, (void *) arg);
    int flag = bufferevent_socket_connect(bev, (struct sockaddr *) &srv, sizeof(srv));
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    if (-1 == flag) {

        //printf("Connect failed retry:%d\n",retry_num );  
        printf("Connect failed retryn");
        /*
        if(retry_num <= 10000){
            retry_num ++;
            bufferevent_free(bev);  
            event_base_free(base);  
            subscribe(address, port, msg, callback);
        }
        */
        return 1;
    }

    event_base_dispatch(base);
    event_base_free(base);
    return 1;

}


void conn_eventcb(struct bufferevent *bev, short events, void *user_data) {
    if (events & BEV_EVENT_EOF) {
        printf("Connection closed ,retrying\n");
        subscribe((NSQArg *) user_data);
    } else if (events & BEV_EVENT_ERROR) {
        printf("Got an error on the connection: %s, retry agin\n", strerror(errno));
        //close fd
        sleep(1);
        bufferevent_free(bev);
        subscribe((NSQArg *) user_data);
    } else if (events & BEV_EVENT_CONNECTED) {
        printf("Connect succeed\n");
        struct NSQMsg *msg = ((struct NSQArg *) user_data)->msg;
        char *v = (char *) malloc(4);
        memcpy(v, "  V2", 4);
        bufferevent_write(bev, v, 4);
        free(v);

        nsq_subscribe(bev, msg->topic, msg->channel);

        nsq_ready(bev, msg->rdy);
        return;
    }

    bufferevent_free(bev);
}


void readcb(struct bufferevent *bev, void *arg) {
    struct NSQMsg *msg = ((struct NSQArg *) arg)->msg;
    int auto_finish = msg->auto_finish;
    //zval *nsq_object = ((struct NSQArg *)arg)->nsq_object;
    zend_fcall_info *fci = ((struct NSQArg *) arg)->fci;;
    zend_fcall_info_cache *fcc = ((struct NSQArg *) arg)->fcc;
    errno = 0;
    int i = 0;
    while (1) {

        char *msg_size = malloc(4);
        memset(msg_size, 0x00, 4);
        size_t size_l = bufferevent_read(bev, msg_size, 4);
        readI32((const unsigned char *) msg_size, &msg->size);

        char *message = malloc(msg->size + 1);
        memset(message, 0x00, msg->size);
        int l = bufferevent_read(bev, message, msg->size);
        if (errno) {
            //printf("errno = %d\n", errno); // errno = 33
            //printf("error: %s\n", strerror(errno));
        }
        if (l) {
            msg->message_id = (char *) malloc(17);
            memset(msg->message_id, '\0', 17);
            readI32((const unsigned char *) message, &msg->frame_type);


            if (msg->frame_type == 0) {
                if (msg->size == 15) {
                    bufferevent_write(bev, "NOP\n", strlen("NOP\n"));
                }
            } else if (msg->frame_type == 2) {
                msg->timestamp = (int64_t) ntoh64((const unsigned char *) message + 4);
                readI16((const unsigned char *) message + 12, &msg->attempts);
                memcpy(msg->message_id, message + 14, 16);
                msg->body = (char *) malloc(msg->size - 30 + 1);
                memset(msg->body, '\0', msg->size - 30 + 1);
                memcpy(msg->body, message + 30, msg->size - 30);

                zval retval;
                zval params[2];
                zval msg_object;
                zval message_id;
                zval attempts;
                zval payload;
                zval timestamp;

                object_init_ex(&msg_object, nsq_message_ce);

                //message_id
                zend_string *message_id_str = zend_string_init(msg->message_id, 16, 0);
                ZVAL_STR_COPY(&message_id, message_id_str);
                zend_update_property(nsq_message_ce, &msg_object, ZEND_STRL("message_id"), &message_id TSRMLS_CC);

                //attempts
                ZVAL_LONG(&attempts, msg->attempts);
                zend_update_property(nsq_message_ce, &msg_object, ZEND_STRL("attempts"), &attempts TSRMLS_CC);
                //timestamp
                ZVAL_LONG(&timestamp, msg->timestamp);
                zend_update_property(nsq_message_ce, &msg_object, ZEND_STRL("timestamp"), &timestamp TSRMLS_CC);

                //payload
                zend_string *payload_str = zend_string_init(msg->body, msg->size - 30, 0);
                ZVAL_STR_COPY(&payload, payload_str);
                zend_update_property(nsq_message_ce, &msg_object, ZEND_STRL("payload"), &payload TSRMLS_CC);

                //call function
                ZVAL_OBJ(&params[0], Z_OBJ(msg_object));
                ZVAL_RES(&params[1], ((struct NSQArg *) arg)->bev_res);
                fci->params = params;
                fci->param_count = 2;
                fci->retval = &retval;
                if (zend_call_function(fci, fcc TSRMLS_CC) != SUCCESS) {
                    php_printf("callback function call failed, The message has been retried\n");
                } else {
                    if (auto_finish) {
                        if (EG(exception)) {
                            nsq_requeue(bev, msg->message_id, msg->delay_time);
                            EG(exception) = NULL;
                        }else{
                            nsq_finish(bev, msg->message_id);
                        }
                    }
                }

                //free memory
                zval_dtor(&params[0]);
                //zval_dtor(&params[1]);
                zend_string_release(payload_str);
                //zval_dtor(&msg_object);

                zend_string_release(message_id_str);
                zval_dtor(&timestamp);
                zval_dtor(&retval);
                zval_dtor(&message_id);
                zval_dtor(&attempts);
                zval_dtor(&payload);
                free(msg->body);
            }
            free(msg->message_id);
        } else {
            break;
        }
        free(message);
        free(msg_size);
        if (l == -1) {
            error_handlings("read() error");;
        }
    }

    //close(sock);

    //return 0;
}


void conn_writecb(struct bufferevent *bev, void *user_data) {
}  

