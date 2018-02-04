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

#ifndef STRUGGLE_NSQ_SUB_CLIENT_H
#define STRUGGLE_NSQ_SUB_CLIENT_H

typedef struct NSQMsg {
    const char *topic;
    const char *channel;
    int32_t frame_type;
    int64_t timestamp;
    uint16_t attempts;
    char *message_id;
    int32_t size;
    char *body;
    int rdy;
    int delay_time;
    zend_bool auto_finish;
} NSQMsg;

typedef struct NSQArg {
    NSQMsg *msg;
    zend_resource * bev_res;
    const char *host;
    const char *port;
    zend_fcall_info *fci;
    zend_fcall_info_cache *fcc;
} NSQArg;

//param is the nsqlookeupd's ip and port ,return the socket fd
int subscribe(NSQArg *nsq_arg);

#endif //STRUGGLE_NSQ_SUB_CLIENT_H
