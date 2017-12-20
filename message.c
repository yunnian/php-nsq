#include <php.h>

 zend_class_entry *nsq_message_ce;

static const zend_function_entry nsq_message_functions[] = {
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
}
