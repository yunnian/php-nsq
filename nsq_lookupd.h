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

#ifndef STRUGGLE_NSQ_LOOKUP_CLIENT_C_H
#define STRUGGLE_NSQ_LOOKUP_CLIENT_C_H

char * request(char * url);

void lookupd_init();

char *lookup(char *host, char *topic);

char *request(char *url);

#endif
