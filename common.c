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
#include "php.h"
#include <arpa/inet.h>

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
