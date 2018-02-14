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

#ifndef STRUGGLE_NSQ_COMMON_H
#define STRUGGLE_NSQ_COMMON_H
#include <stdint.h>

int readI16(const unsigned char *pData, uint16_t *pValue); 

int readI32(const unsigned char *pData, int32_t *pValue);

int readI64(const unsigned char *data, int64_t *pValue); 

uint64_t ntoh64(const uint8_t *data); 

int check_ipaddr(const char *ip);

#endif //STRUGGLE_NSQ_SUB_CLIENT_H
