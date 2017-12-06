#include "php_nsq.h"
#include "nsq_lookupd.h"

const zend_function_entry nsq_lookupd_functions[] = {
	PHP_FE_END	/* Must be the last line in nsq_functions[] */

};
void lookupd_init(){
    zend_class_entry nsq_lookupd;
    INIT_CLASS_ENTRY(nsq_lookupd,"NsqLookupd",nsq_lookupd_functions);
    //nsq_lookupd_ce = zend_register_internal_class_ex(&nsq_lookupd,NULL,NULL TSRMLS_CC);
    nsq_lookupd_ce = zend_register_internal_class(&nsq_lookupd TSRMLS_CC);
}
