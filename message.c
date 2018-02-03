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

#include <php.h>
#include <event2/bufferevent.h>
#include "ext/standard/php_var.h"
#include "command.h"

zend_class_entry *nsq_message_ce;

extern int le_bufferevent;

PHP_METHOD(NsqMessage,touch)
{
    zval *bev_zval;
    zval *message_id;
	ZEND_PARSE_PARAMETERS_START(2,2)
        Z_PARAM_RESOURCE(bev_zval)
        Z_PARAM_ZVAL(message_id)
	ZEND_PARSE_PARAMETERS_END();
    struct bufferevent *bev = (struct bufferevent*)zend_fetch_resource(Z_RES_P(bev_zval), "buffer event", le_bufferevent);
    nsq_touch(bev, Z_STRVAL_P(message_id));
}



PHP_METHOD(NsqMessage,finish)
{
    zval *bev_zval;
    zval *message_id;
	ZEND_PARSE_PARAMETERS_START(2,2)
        Z_PARAM_RESOURCE(bev_zval)
        Z_PARAM_ZVAL(message_id)
	ZEND_PARSE_PARAMETERS_END();
    struct bufferevent *bev = (struct bufferevent*)zend_fetch_resource(Z_RES_P(bev_zval), "buffer event", le_bufferevent);
    nsq_finish(bev, Z_STRVAL_P(message_id));
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_nsq_touch, 0, 0, -1)
    ZEND_ARG_INFO(0, bev_zval)
    ZEND_ARG_INFO(0, message_id)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nsq_finish, 0, 0, -1)
    ZEND_ARG_INFO(0, bev_zval)
    ZEND_ARG_INFO(0, message_id)
ZEND_END_ARG_INFO()

static const zend_function_entry nsq_message_functions[] = {
    PHP_ME(NsqMessage, touch, arginfo_nsq_touch, ZEND_ACC_PUBLIC)
    PHP_ME(NsqMessage, finish, arginfo_nsq_finish, ZEND_ACC_PUBLIC)
    PHP_FE_END	/* Must be the last line in nsq_functions[] */

};

void message_init(){
    zend_class_entry nsq_message;
    INIT_CLASS_ENTRY(nsq_message,"NsqMessage",nsq_message_functions);
    nsq_message_ce = zend_register_internal_class(&nsq_message TSRMLS_CC);
    zend_declare_property_null(nsq_message_ce,ZEND_STRL("message_id"),ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(nsq_message_ce,ZEND_STRL("timestamp"),ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(nsq_message_ce,ZEND_STRL("attempts"),ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(nsq_message_ce,ZEND_STRL("payload"),ZEND_ACC_PUBLIC TSRMLS_CC);
}
