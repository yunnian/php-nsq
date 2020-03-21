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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/php_var.h"
#include "ext/standard/info.h"
#include "ext/json/php_json.h"
#include "php_nsq.h"
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <signal.h>
#include <sys/wait.h>

#include "ext/standard/php_string.h"
#include "sub.h"
#include "pub.h"
#include "message.h"
#include "nsq_lookupd.h"
#include "zend_exceptions.h"
//#include <sys/prctl.h>

#ifdef HAVE_SYS_WAIT_H
#include "sys/wait.h"
#endif

#define PHP_NSQ_REGISTER_CONSTANT(_name, _value) \
	REGISTER_LONG_CONSTANT(_name,  _value, CONST_CS | CONST_PERSISTENT);

typedef enum {
    PHP_NSQ_ERROR_NONE = 0,
    PHP_NSQ_ERROR_NO_CONNECTION,
    PHP_NSQ_ERROR_UNABLE_TO_PUBLISH_MESSAGE,
    PHP_NSQ_ERROR_TOPIC_KEY_REQUIRED,
    PHP_NSQ_ERROR_CHANNEL_KEY_REQUIRED,
    PHP_NSQ_ERROR_LOOKUPD_SERVER_NOT_AVAILABLE,
} php_nsq_error_code;

static const char *php_nsq_get_error_msg(php_nsq_error_code error_code) /* {{{ */
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
        default:
            return "Unknown error";
    }
}
/* }}} */

/* If you declare any globals in php_nsq.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(nsq)
*/

/* True global resources - no need for thread safety here */
static int le_nsq;
int le_bufferevent;

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

zend_class_entry *nsq_ce/*, *nsq_message_exception*/;
PHPAPI zend_class_entry *php_nsq_exception_ce;

static void signal_handle(int sig);

PHP_METHOD(Nsq, __construct){
    zval *self;
    zval *nsq_config  = (zval *)malloc(sizeof(zval)); //use send IDENTIFY comand
    bzero(nsq_config, sizeof(zval));
    ZVAL_NULL(nsq_config);
    self = getThis();
    ZEND_PARSE_PARAMETERS_START(0,1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(nsq_config)
    ZEND_PARSE_PARAMETERS_END();

    if(Z_TYPE_P(nsq_config) != IS_NULL){
        zend_update_property(Z_OBJCE_P(self),self,ZEND_STRL("nsqConfig"), nsq_config TSRMLS_CC);
    }
}



PHP_METHOD (Nsq, connectNsqd)
{
    zval *connect_addr_arr;
    zval *val;
    zval explode_re;

    zend_string *delim = zend_string_init(":", sizeof(":") - 1, 0);
    ZEND_PARSE_PARAMETERS_START(1, 1)
            Z_PARAM_ARRAY(connect_addr_arr)
    ZEND_PARSE_PARAMETERS_END();

    int count = zend_array_count(Z_ARRVAL_P(connect_addr_arr));
    nsqd_connect_config *connect_config_arr = emalloc(count * sizeof(nsqd_connect_config));
    memset(connect_config_arr, 0, count * sizeof(nsqd_connect_config));
    int j = 0, i, h;
    zval *host, *port;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(connect_addr_arr), val) {
        array_init(&explode_re);
        php_explode(delim, Z_STR_P(val), &explode_re, 1);
        host = zend_hash_index_find(Z_ARRVAL_P(&explode_re), 0);
        port = zend_hash_index_find(Z_ARRVAL_P(&explode_re), 1);
        connect_config_arr->port = emalloc(Z_STRLEN_P(port));
        connect_config_arr->host = emalloc(Z_STRLEN_P(host));
        strcpy(connect_config_arr->host, Z_STRVAL_P(host));
        strcpy(connect_config_arr->port, Z_STRVAL_P(port));
        j++;
        if (j < count) {
            connect_config_arr++;
        }
        zval_dtor(&explode_re);

    } ZEND_HASH_FOREACH_END();
    int * sock_arr = connect_nsqd(getThis(), connect_config_arr, count);

    for (h = 0; h < count; h++) {
        efree(connect_config_arr->host);
        efree(connect_config_arr->port);
        if (h < count - 1) {
            connect_config_arr--;
        }

    }
    efree(connect_config_arr);

    zend_string_release(delim);
    //zval_dtor(host);
    //zval_dtor(val);
    //zval_dtor(port);
    int sock_is_true = 1;
    for (i = 0; i < count; i++) {
        if (!(sock_arr[i] > 0)) {
            sock_is_true = 0;
        }
    }
    efree(sock_arr);
    if(sock_is_true){
        RETURN_TRUE;
    }else{
        RETURN_FALSE;
    }
}

PHP_METHOD (Nsq, closeNsqdConnection)
{
    zval *connection_fds;
    zval rv3;
    zval *val;
    connection_fds = zend_read_property(Z_OBJCE_P(getThis()), getThis(), "nsqd_connection_fds", sizeof("nsqd_connection_fds") - 1,
                            1, &rv3);
    int count = zend_array_count(Z_ARRVAL_P(connection_fds));
    if(count == 0){
        zend_throw_exception(php_nsq_exception_ce, php_nsq_get_error_msg(PHP_NSQ_ERROR_NO_CONNECTION), PHP_NSQ_ERROR_NO_CONNECTION);
    }
    int close_success = 1;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(connection_fds), val) {
        if(Z_LVAL_P(val)>0){
            int success = close(Z_LVAL_P(val));
            if(success != 0){
                close_success = 0;
            }
        };
    } ZEND_HASH_FOREACH_END();
    zval_ptr_dtor(connection_fds);
    ZVAL_NULL(connection_fds);
    if(close_success){
        RETURN_TRUE;
    }else{
        RETURN_FALSE;
    }
}

PHP_METHOD (Nsq, publish)
{
    zval *topic;
    zval *msg;
    zval *val;
    zval *sock;
    zval rv3;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(topic)
        Z_PARAM_ZVAL(msg)
    ZEND_PARSE_PARAMETERS_END();
    val = zend_read_property(Z_OBJCE_P(getThis()), getThis(), "nsqd_connection_fds", sizeof("nsqd_connection_fds") - 1,
                             1, &rv3);
    int count = zend_array_count(Z_ARRVAL_P(val));
    if(count == 0){
        zend_throw_exception(php_nsq_exception_ce, php_nsq_get_error_msg(PHP_NSQ_ERROR_UNABLE_TO_PUBLISH_MESSAGE), PHP_NSQ_ERROR_UNABLE_TO_PUBLISH_MESSAGE);
    }
    int r = rand() % count;
    sock = zend_hash_index_find(Z_ARRVAL_P(val), r);

    convert_to_string(msg);
    int re = publish(Z_LVAL_P(sock), Z_STRVAL_P(topic), Z_STRVAL_P(msg));
    //zval_dtor(&rv3);
    //zval_ptr_dtor(msg);
    //zval_dtor(sock);
    if (re > 0) {
        RETURN_TRUE
    } else {
        RETURN_FALSE

    }
}

PHP_METHOD (Nsq, deferredPublish)
{
    zval *topic;
    zval *delay_time;
    zval *msg;
    zval *val;
    zval *sock;
    zval rv3;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_ZVAL(topic)
        Z_PARAM_ZVAL(msg)
        Z_PARAM_ZVAL(delay_time)
    ZEND_PARSE_PARAMETERS_END();
    val = zend_read_property(Z_OBJCE_P(getThis()), getThis(), "nsqd_connection_fds", sizeof("nsqd_connection_fds") - 1,
                             1, &rv3);
    int count = zend_array_count(Z_ARRVAL_P(val));
    if(count == 0){
        zend_throw_exception(php_nsq_exception_ce, php_nsq_get_error_msg(PHP_NSQ_ERROR_UNABLE_TO_PUBLISH_MESSAGE), PHP_NSQ_ERROR_UNABLE_TO_PUBLISH_MESSAGE);
    }
    int r = rand() % count;
    sock = zend_hash_index_find(Z_ARRVAL_P(val), r);

    convert_to_string(msg);
    int re = deferredPublish(Z_LVAL_P(sock), Z_STRVAL_P(topic), Z_STRVAL_P(msg), Z_LVAL_P(delay_time));
    if (re > 0) {
        RETURN_TRUE
    } else {
        RETURN_FALSE

    }
}


HashTable *child_fd;
int is_init = 0;
pid_t master = 0;
ArgPidArr *arg_arr;
int nsqd_num ;

void start_worker_process(NSQArg arg, int index)
{

    zval zval_pid;
    zval zval_arg;
    pid_t pid;
    pid = fork();

    if (pid == 0) {
        subscribe(&arg);
    }else if(pid > 0 ){
        if(!is_init) {
            master = getpid();
            signal(SIGCHLD, signal_handle);
            signal(SIGTERM, signal_handle);

            // init hash table
            ALLOC_HASHTABLE(child_fd);
            zend_hash_init(child_fd, 0, NULL, ZVAL_PTR_DTOR, 1);
            is_init = 1;
        
        }

        arg_arr[index].pid = pid;
        arg_arr[index].arg = arg;

        ZVAL_LONG(&zval_pid, pid);
        zval *fd_res = zend_hash_next_index_insert(child_fd, &zval_pid);
    }
}


static void signal_handle(int sig)
{
    int status;
    pid_t pid;
    zend_ulong index;
    zval *val;
    int count;
    pid_t current = getpid();
    switch (sig)
    {
    case SIGTERM:
        if(current == master){
            count = zend_array_count(child_fd);
              // quit all child
            ZEND_HASH_FOREACH_NUM_KEY_VAL(child_fd, index, val);
                kill(Z_LVAL_P(val), SIGTERM);
            ZEND_HASH_FOREACH_END();
        }

        exit(0);
        break;
        /**
         * TODO reload all workers
         */
    case SIGUSR1:
        break;
    case SIGCHLD:
        /*
        while((pid = waitpid(-1, &status, WNOHANG)) > 0){
            printf("child %d terminated, will reload \n", pid);
            int i;
            for(i = 0; i < nsqd_num; i++){
                if(arg_arr[i].pid == pid){
                    struct NSQArg arg = arg_arr[i].arg;
                    start_worker_process(arg, i);
                }
            }
      };
         */
      break;
    case SIGALRM:
        break;
    default:
        break;
    }
}


PHP_METHOD (Nsq, subscribe)
{
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    zval *config;
    zval *class_lookupd;
    zval *lookupd_addr, rv3, lookupd_re;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_OBJECT(class_lookupd)
        Z_PARAM_ARRAY(config)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END();


    lookupd_addr = zend_read_property(Z_OBJCE_P(class_lookupd), class_lookupd, "address", sizeof("address") - 1, 1,
                                      &rv3);

    zval *topic = zend_hash_str_find(Z_ARRVAL_P(config), "topic", sizeof("topic") - 1);
    if (!topic) {
        zend_throw_exception(php_nsq_exception_ce, php_nsq_get_error_msg(PHP_NSQ_ERROR_TOPIC_KEY_REQUIRED), PHP_NSQ_ERROR_TOPIC_KEY_REQUIRED);
        return;
    }

    zval *channel = zend_hash_str_find(Z_ARRVAL_P(config), "channel", sizeof("channel") - 1);
    if (!channel) {
        zend_throw_exception(php_nsq_exception_ce, php_nsq_get_error_msg(PHP_NSQ_ERROR_CHANNEL_KEY_REQUIRED), PHP_NSQ_ERROR_CHANNEL_KEY_REQUIRED);
    }

    zval *rdy = zend_hash_str_find(Z_ARRVAL_P(config), "rdy", sizeof("rdy") - 1);
    zval *delay_time = zend_hash_str_find(Z_ARRVAL_P(config), "retry_delay_time", sizeof("retry_delay_time") - 1);
    zval *connect_num = zend_hash_str_find(Z_ARRVAL_P(config), "connect_num", sizeof("connect_num") - 1);
    if (!connect_num) {
        connect_num = emalloc(sizeof(zval));
        ZVAL_LONG(connect_num, 1);
    }

    zval *auto_finish = zend_hash_str_find(Z_ARRVAL_P(config), "auto_finish", sizeof("auto_finish") - 1);
    if (auto_finish && Z_TYPE_P(auto_finish) == IS_FALSE) {

        ZVAL_FALSE(auto_finish);

    } else if (auto_finish && Z_TYPE_P(auto_finish) == IS_TRUE) {

        ZVAL_TRUE(auto_finish);

    } else {

        auto_finish = emalloc(sizeof(zval));
        ZVAL_TRUE(auto_finish);
    }

    char *lookupd_re_str; 
    zval * producers;
    zval *message;
    int producers_count;

lookup:
    lookupd_re_str = lookup(Z_STRVAL_P(lookupd_addr), Z_STRVAL_P(topic));

    if (*lookupd_re_str == '\0') {
        zend_throw_exception(php_nsq_exception_ce, php_nsq_get_error_msg(PHP_NSQ_ERROR_LOOKUPD_SERVER_NOT_AVAILABLE), PHP_NSQ_ERROR_LOOKUPD_SERVER_NOT_AVAILABLE);
        return;
    };

    php_json_decode(&lookupd_re, lookupd_re_str, strlen(lookupd_re_str), 1, PHP_JSON_PARSER_DEFAULT_DEPTH);
    producers = zend_hash_str_find(Z_ARRVAL(lookupd_re), "producers", sizeof("producers") - 1);
    if (!producers) {
        message = zend_hash_str_find(Z_ARRVAL(lookupd_re), "message", sizeof("message") - 1);
        php_printf("%s\n", Z_STRVAL_P(message));
        return;

    }

    producers_count = zend_array_count(Z_ARRVAL_P(producers));
    nsqd_num = producers_count;

    if(producers_count < 1){
        //TODO: this maybe exception?
        php_printf("The topic '%s' has not produced on any nsqd in the cluster but are present in the lookup data. The program will be retried after 10 seconds \n",Z_STRVAL_P(topic));
        sleep(10);
        goto lookup;
    
    }

    arg_arr = (struct ArgPidArr*) emalloc(sizeof(ArgPidArr) * producers_count);
    memset(arg_arr, 0, producers_count * sizeof(ArgPidArr));

    // foreach producers  to get nsqd address
    zval *val;
    pid_t pid;
    int i, j;
    j = 0;

    
    for (i = 0; i < Z_LVAL_P(connect_num); i++) {

        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(producers), val) {

            zval *nsqd_host = zend_hash_str_find(Z_ARRVAL_P(val), "broadcast_address", sizeof("broadcast_address") - 1);
            zval *nsqd_port = zend_hash_str_find(Z_ARRVAL_P(val), "tcp_port", sizeof("tcp_port") - 1);
            struct NSQMsg *msg;
            msg = (struct  NSQMsg*) emalloc(sizeof(NSQMsg));
            memset(msg, 0,  sizeof(NSQMsg));
            msg->topic = Z_STRVAL_P(topic);
            msg->channel = Z_STRVAL_P(channel);

            if (rdy) {
                msg->rdy = Z_LVAL_P(rdy);
            } else {
                msg->rdy = 1;
            }

            if (delay_time) {
                msg->delay_time = Z_LVAL_P(delay_time);
            } else {
                msg->delay_time = 0;
            }

            if (auto_finish && Z_TYPE_P(auto_finish) == IS_TRUE) {
                msg->auto_finish = 1;
            } else {
                msg->auto_finish = 0;
            }

            convert_to_string(nsqd_port);

            NSQArg arg;
            arg.msg = msg;
            
            arg.host = Z_STRVAL_P(nsqd_host);
            arg.port = Z_STRVAL_P(nsqd_port);
            arg.fci = &fci;
            arg.fcc = &fcc;
            arg.nsq_obj = getThis();

            start_worker_process(arg, j);

            j++;

        }ZEND_HASH_FOREACH_END();

    }
    int ret_pid = 0;
    while (1) {
        ret_pid = wait(NULL);
        if (ret_pid == -1) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        printf("child process pids %d be terminated, trying reload \n", ret_pid);
        int i ,j ;
        int k = 0;
        printf("last all pid:\n");
        for(i = 0; i < Z_LVAL_P(connect_num); i++){
            for(j = 0; j < nsqd_num; j++){
                printf("    %d\n", arg_arr[k].pid);
                if(arg_arr[k].pid == ret_pid){
                    struct NSQArg arg = arg_arr[k].arg;
                    start_worker_process(arg, k);
                }
                k++;
            }
       }


    }

    zval_dtor(auto_finish);
    zval_dtor(connect_num);
    zval_dtor(config);
    zval_dtor(&lookupd_re);
}


static void _php_bufferevent_dtor(zend_resource *rsrc TSRMLS_DC) /* {{{ */ {
    struct bufferevent *bevent = (struct bufferevent *) rsrc;
    efree(bevent);
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

ZEND_BEGIN_ARG_INFO_EX(arginfo_nsq_ctor, 0, 0, -1)
    ZEND_ARG_INFO(0, nsq_config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nsq_connect_nsqd, 0, 0, -1)
    ZEND_ARG_INFO(0, connect_addr_arr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_close_nsqd_connection, 0, 0, -1)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_nsq_subscribe, 0, 0, -1)
    ZEND_ARG_INFO(0, conifg)
    ZEND_ARG_INFO(0, callback)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(arginfo_nsq_publish, 0, 0, -1)
    ZEND_ARG_INFO(0, topic)
    ZEND_ARG_INFO(0, msg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nsq_d_publish, 0, 0, -1)
    ZEND_ARG_INFO(0, topic)
    ZEND_ARG_INFO(0, msg)
    ZEND_ARG_INFO(0, delay_time)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nsq_requeue, 0, 0, -1)
    ZEND_ARG_INFO(0, delay_time)
ZEND_END_ARG_INFO()

const zend_function_entry nsq_functions[] = {
    PHP_ME(Nsq, __construct, arginfo_nsq_ctor, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(Nsq, connectNsqd, arginfo_nsq_connect_nsqd, ZEND_ACC_PUBLIC)
    PHP_ME(Nsq, closeNsqdConnection, arginfo_close_nsqd_connection, ZEND_ACC_PUBLIC)
    PHP_ME(Nsq, publish, arginfo_nsq_publish, ZEND_ACC_PUBLIC)
    PHP_ME(Nsq, deferredPublish, arginfo_nsq_d_publish, ZEND_ACC_PUBLIC)
    PHP_ME(Nsq, subscribe, arginfo_nsq_subscribe, ZEND_ACC_PUBLIC)
    PHP_FE_END    /* Must be the last line in nsq_functions[] */
};
/* }}} */


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION (nsq)
{
    zend_class_entry nsq;
    INIT_CLASS_ENTRY(nsq, "Nsq", nsq_functions);

    zend_class_entry ne;
    INIT_CLASS_ENTRY(ne, "NsqException", NULL);
    php_nsq_exception_ce = zend_register_internal_class_ex(&ne, zend_ce_exception);

    PHP_NSQ_REGISTER_CONSTANT("NSQ_ERROR_NONE", PHP_NSQ_ERROR_NONE);
    PHP_NSQ_REGISTER_CONSTANT("NSQ_ERROR_NO_CONNECTION", PHP_NSQ_ERROR_NO_CONNECTION);
    PHP_NSQ_REGISTER_CONSTANT("NSQ_ERROR_UNABLE_TO_PUBLISH_MESSAGE", PHP_NSQ_ERROR_UNABLE_TO_PUBLISH_MESSAGE);
    PHP_NSQ_REGISTER_CONSTANT("NSQ_ERROR_TOPIC_KEY_REQUIRED", PHP_NSQ_ERROR_TOPIC_KEY_REQUIRED);
    PHP_NSQ_REGISTER_CONSTANT("NSQ_ERROR_CHANNEL_KEY_REQUIRED", PHP_NSQ_ERROR_CHANNEL_KEY_REQUIRED);
    PHP_NSQ_REGISTER_CONSTANT("NSQ_ERROR_LOOKUPD_SERVER_NOT_AVAILABLE", PHP_NSQ_ERROR_LOOKUPD_SERVER_NOT_AVAILABLE);

    nsq_ce = zend_register_internal_class(&nsq TSRMLS_CC);
    zend_declare_property_null(nsq_ce,ZEND_STRL("nsqConfig"),ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(nsq_ce, ZEND_STRL("nsqd_connection_fds"), ZEND_ACC_PUBLIC TSRMLS_CC);
    zend_declare_property_null(nsq_ce, ZEND_STRL("conn_timeout"), ZEND_ACC_PUBLIC TSRMLS_CC);
    le_bufferevent = zend_register_list_destructors_ex(_php_bufferevent_dtor, NULL, "buffer event", module_number);
    lookupd_init();
    nsq_message_init();

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION (nsq)
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
PHP_RINIT_FUNCTION (nsq)
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
PHP_RSHUTDOWN_FUNCTION (nsq)
{
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION (nsq)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "nsq support", "enabled");
    php_info_print_table_row(2, "version", PHP_NSQ_VERSION);
    php_info_print_table_row(2, "author", "zhenyu.wu[email:wuzhenyu@kuangjue.com]");
    php_info_print_table_end();

    /* Remove comments if you have entries in php.ini
    DISPLAY_INI_ENTRIES();
    */
}
/* }}} */

static const zend_module_dep nsq_deps[] = {
    ZEND_MOD_REQUIRED("json")
    ZEND_MOD_END
};

/* {{{ nsq_module_entry
 */
zend_module_entry nsq_module_entry = {
    STANDARD_MODULE_HEADER_EX,
    NULL,
    nsq_deps,
    "nsq",
    NULL, //nsq_functions,
    PHP_MINIT(nsq),
    PHP_MSHUTDOWN(nsq),
    PHP_RINIT(nsq),        /* Replace with NULL if there's nothing to do at request start */
    PHP_RSHUTDOWN(nsq),    /* Replace with NULL if there's nothing to do at request end */
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
