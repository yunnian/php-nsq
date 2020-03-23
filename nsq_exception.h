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

#ifndef STRUGGLE_NSQ_EXCEPTION_CLIENT_C_H
#define STRUGGLE_NSQ_EXCEPTION_CLIENT_C_H

typedef enum {
    PHP_NSQ_ERROR_NONE = 0,
    PHP_NSQ_ERROR_NO_CONNECTION,
    PHP_NSQ_ERROR_UNABLE_TO_PUBLISH_MESSAGE,
    PHP_NSQ_ERROR_TOPIC_KEY_REQUIRED,
    PHP_NSQ_ERROR_CHANNEL_KEY_REQUIRED,
    PHP_NSQ_ERROR_LOOKUPD_SERVER_NOT_AVAILABLE,
    PHP_NSQ_ERROR_PUB_LOST_CONNECTION,
    PHP_NSQ_ERROR_TOPIC_NOT_EXISTS,
    PHP_NSQ_ERROR_CALLBACK_FUNCTION_IS_NOT_CALLABLE,
    PHP_NSQ_ERROR_LIBEVENT_COULD_NOT_BE_INITIALIZED,
    PHP_NSQ_ERROR_CONNECTION_FAILED,
} php_nsq_error_code;

static const char *php_nsq_get_error_msg(php_nsq_error_code error_code);

void nsq_exception_init();

void throw_exception(php_nsq_error_code error_code);

#endif
