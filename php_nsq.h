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

#ifndef PHP_NSQ_H
#define PHP_NSQ_H

extern zend_module_entry nsq_module_entry;
#define phpext_nsq_ptr &nsq_module_entry

#define PHP_NSQ_VERSION "3.9.1" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_NSQ_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_NSQ_API __attribute__ ((visibility("default")))
#else
#	define PHP_NSQ_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define get_object_handle(object)        Z_OBJ_HANDLE_P(object)



void error_handlings(char *message);

void error_handlings(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    //exit(1);
}


/*
  	Declare any global variables you may need between the BEGIN
	and END macros here:

ZEND_BEGIN_MODULE_GLOBALS(nsq)
	zend_long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(nsq)
*/

/* Always refer to the globals in your function as NSQ_G(variable).
   You are encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/
#define NSQ_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(nsq, v)

#if defined(ZTS) && defined(COMPILE_DL_NSQ)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif    /* PHP_NSQ_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
