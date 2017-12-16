/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
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
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/php_var.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/json/php_json.h"
#include "php_nsq.h"
#include <event2/bufferevent.h>  
#include <event2/buffer.h>  
#include <event2/listener.h>  
#include <event2/util.h>  
#include <event2/event.h>  

#include <sub.h>  


/* If you declare any globals in php_nsq.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(nsq)
*/

/* True global resources - no need for thread safety here */
static int le_nsq;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("nsq.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_nsq_globals, nsq_globals)
    STD_PHP_INI_ENTRY("nsq.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_nsq_globals, nsq_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_nsq_compiled(string arg)
   Return a string to confirm that the module is compiled in */
zend_class_entry *nsq_ce;

int msg_callback_m(NSQMsg *msg){

    printf("test message handler:%s\n", msg->body);
    return 0;
}
void* handler(int sig) {
    int status;
 
    if (waitpid(-1, &status, WNOHANG) >= 0)
    {
        printf("child is die,i know\n");
    }
}
PHP_METHOD(Nsq,subscribe)
{
    struct event_base *base = event_base_new();  
	zend_fcall_info  fci;
	zend_fcall_info_cache fcc;
	zval *config;
    zval *class_lookupd;
    zval *lookupd_addr,rv3,lookupd_re;

	ZEND_PARSE_PARAMETERS_START(3,3)
        Z_PARAM_OBJECT(class_lookupd)
        Z_PARAM_ARRAY(config)
		Z_PARAM_FUNC(fci, fcc)
	ZEND_PARSE_PARAMETERS_END();


    lookupd_addr = zend_read_property(Z_OBJCE_P(class_lookupd), class_lookupd, "address", sizeof("address")-1, 1, &rv3);

    zval * topic = zend_hash_str_find(Z_ARRVAL_P(config),"topic",sizeof("topic")-1);
    zval * channel = zend_hash_str_find(Z_ARRVAL_P(config),"channel",sizeof("channel")-1);
    zval * rdy = zend_hash_str_find(Z_ARRVAL_P(config),"rdy",sizeof("rdy")-1);
    zval * connect_num  = zend_hash_str_find(Z_ARRVAL_P(config),"connect_num",sizeof("connect_num")-1);
    char * lookupd_re_str = lookup(Z_STRVAL_P(lookupd_addr), Z_STRVAL_P(topic));
    php_json_decode(&lookupd_re, lookupd_re_str, sizeof(lookupd_re_str)-1,1,PHP_JSON_PARSER_DEFAULT_DEPTH);
    zval * producers = zend_hash_str_find(Z_ARRVAL(lookupd_re),"producers",sizeof("producers")-1);

    printf("connect_num----------:%d",Z_LVAL_P(connect_num));
        
        
    // foreach producers  to get nsqd address
    zval * val;
    pid_t pid, wt;
    for (int i = 1; i <= Z_LVAL_P(connect_num); i++) {

    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(producers), val) {
		//signal(SIGCHLD,handler);
        pid = fork();

        if(pid == 0){
            zval * nsqd_host = zend_hash_str_find(Z_ARRVAL_P(val),"broadcast_address",sizeof("broadcast_address")-1);
            zval * nsqd_port = zend_hash_str_find(Z_ARRVAL_P(val),"tcp_port",sizeof("tcp_port")-1);
            struct NSQMsg *msg;
            msg = malloc(sizeof(NSQMsg));
            msg->topic = Z_STRVAL_P(topic);
            msg->channel = Z_STRVAL_P(channel); 
            msg->rdy = Z_LVAL_P(rdy);
            //int (*msg_callback)(struct NSQMsg *msg) = msg_callback_m;
            printf("host:%s",Z_STRVAL_P(nsqd_host));
            printf("port:%ld", Z_LVAL_P(nsqd_port));
            convert_to_string(nsqd_port);
            printf("port:%s", Z_STRVAL_P(nsqd_port));

            subscribe("127.0.0.1", Z_STRVAL_P(nsqd_port), msg, &fci, &fcc); //现在只是 直连nsqd 的地址,lookupd地址支持 以后上

            //int re = subscribe(sock, msg, msg_callback);
            //printf("re:%d",re);
            free(msg);
        }

        
        
    } ZEND_HASH_FOREACH_END();

    php_var_dump(config, 1);
    zval_dtor(config);
    }
	wt = wait(NULL);
	printf("nihao yituichu");

    //server_values = zend_hash_find(Z_ARRVAL_P(return_value), server_key);
	//printf("nihao:%s",test);

	
    // json 解析
    //zval  test;
    //char * s = "{\"message\":\"NOT_FOUND\"}";
    //php_json_decode(&test, s, strlen(s),1,2);
    //zend_array *arr = test.value.arr;
    //php_var_dump(test, 1);


    zval_dtor(&lookupd_re);

	//RETURN_STR(strg);
    //add_index_zval(return_value,333,config);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_nsq_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_nsq_init_globals(zend_nsq_globals *nsq_globals)
{
	nsq_globals->global_value = 0;
	nsq_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ nsq_functions[]
 *
 * Every user visible function must have an entry in nsq_functions[].
 */
 ZEND_BEGIN_ARG_INFO_EX(arginfo_nsq_subscribe, 0, 0, -1)
    ZEND_ARG_INFO(0, conifg)
    ZEND_ARG_INFO(0, callback)
ZEND_END_ARG_INFO()

const zend_function_entry nsq_functions[] = {
	//PHP_FE(subscribe,	NULL)		/* For testing, remove later. */
    PHP_ME(Nsq, subscribe, arginfo_nsq_subscribe, ZEND_ACC_PUBLIC)
	PHP_FE_END	/* Must be the last line in nsq_functions[] */
};
/* }}} */


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(nsq)
{

    zend_class_entry nsq;
    INIT_CLASS_ENTRY(nsq,"Nsq",nsq_functions);
    nsq_ce = zend_register_internal_class(&nsq TSRMLS_CC);
    //nsq_ce = zend_register_internal_class_ex(&nsq,NULL,NULL TSRMLS_CC);
    //zend_declare_property_null(person_ce,ZEND_STRL("address"),ZEND_ACC_PUBLIC TSRMLS_CC);
    
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/
    lookupd_init();
    //conifg_init();

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(nsq)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(nsq)
{
#if defined(COMPILE_DL_NSQ) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(nsq)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(nsq)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "nsq support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ nsq_module_entry
 */
zend_module_entry nsq_module_entry = {
	STANDARD_MODULE_HEADER,
	"nsq",
	NULL, //nsq_functions,
	PHP_MINIT(nsq),
	PHP_MSHUTDOWN(nsq),
	PHP_RINIT(nsq),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(nsq),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(nsq),
	PHP_NSQ_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_NSQ
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(nsq)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
