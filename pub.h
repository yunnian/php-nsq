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

#ifndef STRUGGLE_NSQ_PUB_CLIENT_C_H
#define STRUGGLE_NSQ_PUB_CLIENT_C_H
typedef struct nsqd_connect_config {
    char *host;
    char *port;

} nsqd_connect_config;

int * connect_nsqd(zval *ce, nsqd_connect_config *connect_config_arr, int connect_num);

int publish(int sock, char *topic, char *msg, size_t msg_len);

int deferredPublish(int sock, char *topic, char *msg, size_t msg_len, int delay_time);

#endif //STRUGGLE_NSQ_PUB_CLIENT_C_H
