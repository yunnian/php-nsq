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
#include <sys/socket.h>
#include "pub.h"
#include "ext/standard/php_var.h"
#include "ext/standard/php_smart_string_public.h"
#include "ext/json/php_json.h"
#include "zend_smart_str.h"

extern void error_handlings(char* message) ;

int ReadI32(const char * pData, int *pValue)
{
    *pValue = (pData[0] << 24) | (pData[1] << 16) | (pData[2] << 8) | pData[3];
    return 0;
}

int send_identify(zval *nsq_obj, int sock)
{

    //IDENTIFY
    zval * nsq_config;
    zval rv3;
    zval json;
    nsq_config = zend_read_property(Z_OBJCE_P(nsq_obj), nsq_obj, "nsqConfig", sizeof("nsqConfig")-1, 1, &rv3);

    smart_str json_buf = {0};
    if(Z_TYPE_P(nsq_config) != IS_NULL){
        php_json_encode(&json_buf, nsq_config, 0) ;   
        smart_str_0(&json_buf);
        ZVAL_NEW_STR(&json,json_buf.s);
        char * identify_command = malloc(256);
        memset(identify_command, '\0', 256);
        int identify_len = sprintf(identify_command, "%s", "IDENTIFY\n");
        uint32_t json_len = htonl(Z_STRLEN(json));
        memcpy(identify_command + identify_len, &json_len, 4);
        int len_2 = sprintf(identify_command + identify_len + 4, "%s", Z_STRVAL(json));
        send(sock,identify_command, identify_len+Z_STRLEN(json)+4 ,0);  
    }
    zval_dtor(&json);
    return 0;
}

int connect_nsqd(zval *nsq_obj, nsqd_connect_config * connect_config_arr, int connect_num){
    int * sock_arr = emalloc(connect_num*sizeof(int));
    zval * fds;
    zval * val;
    zval rv3;
    fds = zend_read_property(Z_OBJCE_P(nsq_obj), nsq_obj, "nsqd_connection_fds", sizeof("nsqd_connection_fds")-1, 1, &rv3);

    if(Z_TYPE_P(fds) != IS_NULL){
        /*
        int i = 0;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(fds), val){
            php_var_dump(val,1);
            sock_arr[i] = Z_LVAL_P(val);
            php_printf("wodefds:%d", sock_arr[i]);
            i++;
        }ZEND_HASH_FOREACH_END();
        */
        return  1;
    
    }


    int i;
    for (i = 0; i < connect_num; i++) {
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        sock_arr[i] = socket(PF_INET, SOCK_STREAM, 0);
        if (sock_arr[i] == -1) {
            error_handlings("socket() error");
        }
        
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(connect_config_arr->host);
        serv_addr.sin_port = htons(atoi(connect_config_arr->port));
        if(i < connect_num-1){
            connect_config_arr-- ;
        }

        if( connect(sock_arr[i],(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1) {
            error_handlings("connect() error");
        }
        char * msgs  = (char * ) malloc(4);
        memcpy(msgs, "  V2", 4);
        int r = write((sock_arr[i]), msgs, 4);  
        send_identify(nsq_obj, sock_arr[i]);
        free(msgs);
    }

    zval fd_arr;
    array_init(&fd_arr);
    for (i = 0; i < connect_num; i++) {
        if(!(sock_arr[i] > 0)){
            return -1;
        }
        zval fd_val;
        ZVAL_LONG(&fd_val, sock_arr[i]);
        zend_hash_index_add(Z_ARRVAL(fd_arr), i, &fd_val);
        //zval_dtor(&fd_val);
    }
    

    zend_update_property(Z_OBJCE_P(nsq_obj),nsq_obj,ZEND_STRL("nsqd_connection_fds"), &fd_arr TSRMLS_CC);
    efree(sock_arr);
    zval_dtor(&fd_arr);
    //zval_dtor(&rv3);
    return 1;

}


int publish(int sock, char *topic, char *msg){

	char buf[1024*1024];
    size_t n;
	char * pub_command = malloc(strlen(topic) + strlen("PUB \n"));
    sprintf(pub_command, "%s%s%s", "PUB ",topic, "\n");
	int  len = htonl(strlen(msg));
    n = sprintf(buf, "%s", pub_command);
	memcpy(&buf[strlen(pub_command)], &len, 4);
    n = sprintf(&buf[strlen(pub_command)+4], "%s", msg);
	int sendLen = strlen(pub_command) + strlen(msg)+4;
	send(sock, buf,sendLen ,0);  
    free(pub_command);

    char * message = malloc(3);
    while(1) {
        memset(message, '\0', 3);
        int l = read(sock, message, 2);
        if(strcmp(message,"OK")==0){
            break;
        }
        if(l == 0){
            break;
        }
    }

    if(strcmp(message,"OK")==0){
        return sock;
    }else{
        return -1;
    }
}

int deferredPublish(int sock, char *topic, char *msg, int defer_time){
	char buf[1024*1024];
    size_t n;
	char * pub_command = malloc(128);
    int command_len = sprintf(pub_command, "%s%s%s%lld%s", "DPUB ", topic, " ", defer_time, "\n");
	int  len = htonl(strlen(msg));
	//int  len = strlen(msg);
    n = sprintf(buf, "%s", pub_command);
	memcpy(&buf[command_len], &len, 4);
    n = sprintf(&buf[command_len + 4], "%s", msg);
	int sendLen = command_len + strlen(msg) + 4;

	send(sock, buf,sendLen ,0);  
    free(pub_command);
    char * message = malloc(3);
    while(1) {
        memset(message, '\0', 3);

        int l = read(sock, message, 2);
        if(strcmp(message,"OK")==0){
            break;
        }
        if(l == 0){
            break;
        }
    }

    if(strcmp(message,"OK")==0){
        return sock;
    }else{
        return -1;
    }
}
