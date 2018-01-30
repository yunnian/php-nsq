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
#include "ext/standard/php_var.h"

zend_class_entry *nsq_message_ce;

extern struct bufferevent bev_resource;
PHP_METHOD(NsqMessage,touch)
{
    zval *consumer_index;
    zval rv3;
    consumer_index = zend_read_property(Z_OBJCE_P(getThis()), getThis(), "consumer_index", sizeof("consumer_index")-1, 1, &rv3);
    php_var_dump(consumer_index,1);
    php_printf("啦拉拉我是卖报的小行家");
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_nsq_touch, 0, 0, -1)
ZEND_END_ARG_INFO()

static const zend_function_entry nsq_message_functions[] = {
    PHP_ME(NsqMessage, touch, arginfo_nsq_touch, ZEND_ACC_PUBLIC)
	PHP_FE_END	/* Must be the last line in nsq_functions[] */

};

void message_init(){
    zend_class_entry nsq_message;
    INIT_CLASS_ENTRY(nsq_message,"NsqMessage",nsq_message_functions);
    //nsq_lookupd_ce = zend_register_internal_class_ex(&nsq_lookupd,NULL,NULL TSRMLS_CC);
    nsq_message_ce = zend_register_internal_class(&nsq_message TSRMLS_CC);
    zend_declare_property_null(nsq_message_ce,ZEND_STRL("message_id"),ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(nsq_message_ce,ZEND_STRL("timestamp"),ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(nsq_message_ce,ZEND_STRL("attempts"),ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(nsq_message_ce,ZEND_STRL("payload"),ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(nsq_message_ce,ZEND_STRL("consumer_index"),ZEND_ACC_PUBLIC TSRMLS_CC);
}
