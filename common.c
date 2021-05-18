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


#include "common.h"
#include <arpa/inet.h>
#include "ext/standard/php_var.h"
#include "ext/standard/info.h"
#include "ext/json/php_json.h"
#include "ext/standard/php_string.h"
#include "zend_smart_str.h"

int send_identify(zval *nsq_obj, int sock)
{
    //IDENTIFY
    zval * nsq_config;
    zval rv3;
    zval json;
    nsq_config = zend_read_property(Z_OBJCE_P(nsq_obj), NSQ_COMPAT_OBJ_P(nsq_obj), "nsqConfig", sizeof("nsqConfig")-1, 1, &rv3);

    smart_str json_buf = {0};
    if(Z_TYPE_P(nsq_config) != IS_NULL){
        php_json_encode(&json_buf, nsq_config, 0) ;   
        smart_str_0(&json_buf);
        ZVAL_NEW_STR(&json,json_buf.s);
        char * identify_command = emalloc(256);
        memset(identify_command, '\0', 256);
        int identify_len = sprintf(identify_command, "%s", "IDENTIFY\n");
        uint32_t json_len = htonl(Z_STRLEN(json));
        memcpy(identify_command + identify_len, &json_len, 4);
        int len_2 = sprintf(identify_command + identify_len + 4, "%s", Z_STRVAL(json));
        send(sock,identify_command, identify_len+Z_STRLEN(json)+4 ,0);  
        zval *negotiation = zend_hash_str_find(Z_ARRVAL_P(nsq_config), "feature_negotiation", sizeof("feature_negotiation") - 1);


        int l = 0;
        int current_l = 0;
        int msg_size;
        char *message;
        char *msg_size_char = malloc(4);
        memset(msg_size_char, 0x00, 4);
        int size;

again_size:
        size = read(sock, msg_size_char, 4);
        if(size <= 0){
            goto again_size;
        }
        readI32((const unsigned char *) msg_size_char, &msg_size);

        free(msg_size_char);

        message = emalloc(msg_size + 1);
        memset(message, 0x00, msg_size);
again:
        l += read(sock, message +l , msg_size);
        if( l < msg_size ){
            goto again;
        
        }

        efree(message);
        efree(identify_command);
        zval_ptr_dtor(nsq_config);
        zval_ptr_dtor(&json);
    }
	
    return 0;
}

int readI16(const unsigned char *pData, uint16_t *pValue) {
    *pValue = (pData[0] << 8) | pData[1];
    return 0;
}

int readI32(const unsigned char *pData, int32_t *pValue) {
    *pValue = (pData[0] << 24) | (pData[1] << 16) | (pData[2] << 8) | pData[3];
    return 0;
}


int readI64(const unsigned char *data, int64_t *pValue) {
    *pValue = ((uint64_t) data[0] << 56) | ((uint64_t) data[1] << 48) | ((uint64_t) data[2] << 40) |
              ((uint64_t) data[3] << 32) | ((uint64_t) data[4] << 24) | ((uint64_t) data[5] << 16) |
              ((uint64_t) data[6] << 8) | (uint64_t) data[7];
    return 0;

}

uint64_t ntoh64(const uint8_t *data) {
    return (uint64_t) (data[7]) | (uint64_t) (data[6]) << 8 |
           (uint64_t) (data[5]) << 16 | (uint64_t) (data[4]) << 24 |
           (uint64_t) (data[3]) << 32 | (uint64_t) (data[2]) << 40 |
           (uint64_t) (data[1]) << 48 | (uint64_t) (data[0]) << 56;
}

int check_ipaddr(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }

    struct sockaddr_in6 addr6;
    struct sockaddr_in addr4;

    if (1 == inet_pton(AF_INET, str, &addr4.sin_addr)) {
        return 1;
    } else if (1 == inet_pton(AF_INET6, str, &addr6.sin6_addr)) {
        return 1;
    }
    return 0;
}
