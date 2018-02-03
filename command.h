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

#ifndef STRUGGLE_NSQ_MESSAGE_CLIENT_C_H
#define STRUGGLE_NSQ_MESSAGE_CLIENT_C_H char * request(char * url);

void nsq_subscribe(struct bufferevent *bev, const char *topic, const char *channel);

void nsq_requeue(struct bufferevent *bev, const char *id, int timeout_ms);

void nsq_finish(struct bufferevent *bev, const char *id);

void nsq_touch(struct bufferevent *bev, const char *id);

void nsq_ready(struct bufferevent *bev, int count);

void nsq_nop(struct bufferevent *bev);

#endif
