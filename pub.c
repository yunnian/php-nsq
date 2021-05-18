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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include "pub.h"
#include "common.h"
#include "ext/standard/php_var.h"
#include <errno.h>
#include <fcntl.h>
#include "ext/standard/php_smart_string_public.h"
#include "ext/json/php_json.h"
#include "zend_smart_str.h"
#include <signal.h>
#include "nsq_exception.h"
#include "zend_exceptions.h"

extern void error_handlings(char *message);
//typedef void (*sighandler_t)(int);
//void respond_hearbeat(int sock);

void nsq_conf_timeout(zval *nsq_obj, struct timeval *timeout)
{
    zval *conn_timeout;
    zval rv3;

    if (!nsq_obj || !timeout)
        return;

    conn_timeout = zend_read_property(Z_OBJCE_P(nsq_obj), NSQ_COMPAT_OBJ_P(nsq_obj), "conn_timeout", sizeof("conn_timeout") - 1, 1, &rv3);
    if(Z_TYPE_P(conn_timeout) != IS_LONG || Z_LVAL_P(conn_timeout) < 0)
        return;

    timeout->tv_sec  = Z_LVAL_P(conn_timeout) / 1000;
    timeout->tv_usec = (Z_LVAL_P(conn_timeout) % 1000) * 1000;
}

int * connect_nsqd(zval *nsq_obj, nsqd_connect_config *connect_config_arr, int connect_num) {
    int *sock_arr = emalloc(connect_num * sizeof(int));
    zval *fds;
    zval *val;
    zval rv3;
    struct hostent *he;
    struct timeval timeout;

    fds = zend_read_property(Z_OBJCE_P(nsq_obj), NSQ_COMPAT_OBJ_P(nsq_obj), "nsqd_connection_fds", sizeof("nsqd_connection_fds") - 1, 1,
                             &rv3);

    if (Z_TYPE_P(fds) != IS_NULL) {
        int i = 0;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(fds), val){
            sock_arr[i] = Z_LVAL_P(val);
            i++;
        }ZEND_HASH_FOREACH_END();
        return sock_arr;
    }

    memset(&timeout, 0, sizeof(timeout));
    nsq_conf_timeout(nsq_obj, &timeout);

    int i;
    for (i = 0; i < connect_num; i++) {
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        sock_arr[i] = socket(PF_INET, SOCK_STREAM, 0);
        if (sock_arr[i] == -1) {
            error_handlings("socket() error");
        }

        serv_addr.sin_family = AF_INET;

        if (check_ipaddr(connect_config_arr->host)) {
            serv_addr.sin_addr.s_addr = inet_addr(connect_config_arr->host);
        } else {
            /* resolve hostname */
            if ((he = gethostbyname(connect_config_arr->host)) == NULL) {
                exit(1); /* error */
            }
            /* copy the network address to sockaddr_in structure */
            memcpy(&serv_addr.sin_addr, he->h_addr_list[0], he->h_length);
        }
        serv_addr.sin_port = htons(atoi(connect_config_arr->port));
        if (i < connect_num - 1) {
            connect_config_arr--;
        }

        if (timeout.tv_usec > 0 || timeout.tv_sec > 0)
            setsockopt(sock_arr[i], SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        if (connect(sock_arr[i], (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
            error_handlings("connect() error");
            sock_arr[i] = 0;
            continue;
        }

        // reset timeout to default behaviour
        if (timeout.tv_usec > 0 || timeout.tv_sec > 0) {
            timeout.tv_sec = 0;
            timeout.tv_usec = 0;
            setsockopt(sock_arr[i], SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        }

        int flags = fcntl(sock_arr[i], F_GETFL, 0);
        fcntl(sock_arr[i], F_SETFL, flags | O_NONBLOCK);
        char *msgs = (char *) emalloc(4);
        memcpy(msgs, "  V2", 4);
        int r = send((sock_arr[i]), msgs, 4, MSG_DONTWAIT);
        send_identify(nsq_obj, sock_arr[i]);
        efree(msgs);
    }

    zval fd_arr;
    array_init(&fd_arr);
    for (i = 0; i < connect_num; i++) {
        if (!(sock_arr[i] > 0)) {
            zval_dtor(&fd_arr);
            return  sock_arr;
        }
        zval fd_val;
        ZVAL_LONG(&fd_val, sock_arr[i]);
        zend_hash_index_add(Z_ARRVAL(fd_arr), i, &fd_val);
        //zval_dtor(&fd_val);
    }


    zend_update_property(Z_OBJCE_P(nsq_obj),  NSQ_COMPAT_OBJ_P(nsq_obj), ZEND_STRL("nsqd_connection_fds"), &fd_arr);
    zval_dtor(&fd_arr);
    //zval_dtor(&rv3);
    return  sock_arr;
}


extern int errno;

int publish(int sock, char *topic, char *msg, size_t msg_len) {
    char buf[1024 * 1024];

    size_t ofs  = 0;
    // write command
    // PUB <topic>/n
    // write command
    strncpy(&buf[ofs], "PUB ", 4);
    ofs+=4;
    // write topic
    strcpy(&buf[ofs], topic);
    ofs+=strlen(topic);
    // write command delimiter
    buf[ofs] = '\n';
    ofs+=1;

    // write msg len
    int len = htonl(msg_len);
    memcpy(&buf[ofs], &len, 4);
    ofs+=4;
    // write msg
    memcpy(&buf[ofs], msg, msg_len);
    ofs+=msg_len;

    send(sock, buf, ofs, 0);

    int l = 0;
    int msg_size;
    char *message;
    char *msg_size_char = malloc(4);
    memset(msg_size_char, 0x00, 4);
    int size;

again_size:
    size = read(sock, msg_size_char, 4);
    if( size ==  0 ){
        throw_exception(PHP_NSQ_ERROR_PUB_LOST_CONNECTION);
        free(msg_size_char);
        return -1;
    }
    if(size == -1){
        goto again_size;
    }
    readI32((const unsigned char *) msg_size_char, &msg_size);

    free(msg_size_char);

    message = emalloc(msg_size + 1);
    memset(message, 0x00, msg_size);
again:
    l += read(sock, message +l , msg_size);
    if( l < msg_size && l>0){
        goto again;

    }
    if (strcmp(message + 4, "OK") == 0) {
        efree(message);
        return sock;
    } else {
        efree(message);
        return -1;
    }

}

int deferredPublish(int sock, char *topic, char *msg, size_t msg_len, int defer_time) {
    char buf[1024 * 1024];

    size_t ofs  = 0;
    // write command
    // DPUB <topic> <defer_time>/n
    // write command
    strncpy(&buf[ofs], "DPUB ", 5);
    ofs+=5;
    // write topic
    strcpy(&buf[ofs], topic);
    ofs+=strlen(topic);
    buf[ofs] = ' ';
    ofs+=1;
    // write defer_time
    int defer_len = sprintf(&buf[ofs], "%lld", defer_time);
    ofs+=defer_len;
    // write command delimiter
    buf[ofs] = '\n';
    ofs+=1;

    // write msg len
    int len = htonl(msg_len);
    memcpy(&buf[ofs], &len, 4);
    ofs+=4;
    // write msg
    memcpy(&buf[ofs], msg, msg_len);
    ofs+=msg_len;

    send(sock, buf, ofs, 0);

    int l = 0;
    int current_l = 0;
    int msg_size;
    char *message;
    char *msg_size_char = malloc(4);
    memset(msg_size_char, 0x00, 4);
    int size;
    /*
    sighandler_t handler = respond_hearbeat ;
    signal(SIGALRM, handler);
    alarm(5);
    */

again_size:
    size = read(sock, msg_size_char , 4);
    if( size ==  0 ){
        throw_exception(PHP_NSQ_ERROR_PUB_LOST_CONNECTION);
        free(msg_size_char);
        return -1;
    }
    if(size == -1){
        goto again_size;
    }
    readI32((const unsigned char *) msg_size_char, &msg_size);

    free(msg_size_char);

    message = emalloc(msg_size + 1);
    memset(message, 0x00, msg_size);
again:
    l += read(sock, message +l , msg_size);
    if( l < msg_size && l>0){
        goto again;

    }

    if (strcmp(message + 4, "OK") == 0) {
        efree(message);
        return sock;
    } else {
        return -1;
    }
}

/*
void respond_hearbeat(int sock){
    //send(sock, "NOP",3 , 0);
}
*/


