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

#include "php.h"
#include "stdio.h"
#include "string.h"
#include "assert.h"
#include "nsq_exception.h"
#include "zend_exceptions.h"

#define PHP_NSQ_EXCEPTION_REGISTER_CONSTANT(_name, _value) \
    zend_register_long_constant((_name), sizeof(_name)-1, (_value), (CONST_CS | CONST_PERSISTENT), 0);

PHPAPI zend_class_entry *php_nsq_exception_ce;

static const char *php_nsq_get_error_msg(php_nsq_error_code error_code)
{
    switch(error_code) {
        case PHP_NSQ_ERROR_NONE:
            return "No error";
        case PHP_NSQ_ERROR_NO_CONNECTION:
            return "No connection to close";
        case PHP_NSQ_ERROR_UNABLE_TO_PUBLISH_MESSAGE:
            return "Unable to publish message";
        case PHP_NSQ_ERROR_TOPIC_KEY_REQUIRED:
            return "Topic key is required";
        case PHP_NSQ_ERROR_CHANNEL_KEY_REQUIRED:
            return "Channel key is required";
        case PHP_NSQ_ERROR_LOOKUPD_SERVER_NOT_AVAILABLE:
            return "Lookupd server not available";
        case PHP_NSQ_ERROR_PUB_LOST_CONNECTION:
            return "Pub lost connection";
        case PHP_NSQ_ERROR_TOPIC_NOT_EXISTS:
            return "Topic not exists";
        case PHP_NSQ_ERROR_CALLBACK_FUNCTION_IS_NOT_CALLABLE:
            return "Callback function in subscribe is not callable";
        case PHP_NSQ_ERROR_LIBEVENT_COULD_NOT_BE_INITIALIZED:
            return "Could not initialize libevent";
        case PHP_NSQ_ERROR_CONNECTION_FAILED:
            return "Connection failed";
        default:
            return "Unknown error";
    }
}

static PHP_METHOD(NsqException, __construct);

PHPAPI zend_class_entry *php_nsq_exception_ce;

void nsq_exception_init() {
    zend_class_entry nsq_exception;

    INIT_CLASS_ENTRY(nsq_exception, "NsqException", NULL);
    php_nsq_exception_ce = zend_register_internal_class_ex(&nsq_exception, zend_ce_exception);

    PHP_NSQ_EXCEPTION_REGISTER_CONSTANT("NSQ_ERROR_NONE", PHP_NSQ_ERROR_NONE);
    PHP_NSQ_EXCEPTION_REGISTER_CONSTANT("NSQ_ERROR_NO_CONNECTION", PHP_NSQ_ERROR_NO_CONNECTION);
    PHP_NSQ_EXCEPTION_REGISTER_CONSTANT("NSQ_ERROR_UNABLE_TO_PUBLISH_MESSAGE", PHP_NSQ_ERROR_UNABLE_TO_PUBLISH_MESSAGE);
    PHP_NSQ_EXCEPTION_REGISTER_CONSTANT("NSQ_ERROR_TOPIC_KEY_REQUIRED", PHP_NSQ_ERROR_TOPIC_KEY_REQUIRED);
    PHP_NSQ_EXCEPTION_REGISTER_CONSTANT("NSQ_ERROR_CHANNEL_KEY_REQUIRED", PHP_NSQ_ERROR_CHANNEL_KEY_REQUIRED);
    PHP_NSQ_EXCEPTION_REGISTER_CONSTANT("NSQ_ERROR_LOOKUPD_SERVER_NOT_AVAILABLE", PHP_NSQ_ERROR_LOOKUPD_SERVER_NOT_AVAILABLE);
    PHP_NSQ_EXCEPTION_REGISTER_CONSTANT("NSQ_ERROR_PUB_LOST_CONNECTION", PHP_NSQ_ERROR_PUB_LOST_CONNECTION);
    PHP_NSQ_EXCEPTION_REGISTER_CONSTANT("NSQ_ERROR_TOPIC_NOT_EXISTS", PHP_NSQ_ERROR_TOPIC_NOT_EXISTS);
    PHP_NSQ_EXCEPTION_REGISTER_CONSTANT("NSQ_ERROR_CALLBACK_FUNCTION_IS_NOT_CALLABLE", PHP_NSQ_ERROR_CALLBACK_FUNCTION_IS_NOT_CALLABLE);
    PHP_NSQ_EXCEPTION_REGISTER_CONSTANT("NSQ_ERROR_LIBEVENT_COULD_NOT_BE_INITIALIZED", PHP_NSQ_ERROR_LIBEVENT_COULD_NOT_BE_INITIALIZED);
    PHP_NSQ_EXCEPTION_REGISTER_CONSTANT("NSQ_ERROR_CONNECTION_FAILED", PHP_NSQ_ERROR_CONNECTION_FAILED);
}

void throw_exception(php_nsq_error_code error_code) {
    zend_throw_exception(php_nsq_exception_ce, php_nsq_get_error_msg(error_code), error_code);
}

PHP_METHOD (NsqException, __construct) {
    zval *self;
    self = getThis();
}

