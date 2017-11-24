dnl $Id$
dnl config.m4 for extension nsq

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(nsq, for nsq support,
dnl Make sure that the comment is aligned:
dnl [  --with-nsq             Include nsq support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(nsq, whether to enable nsq support,
dnl Make sure that the comment is aligned:
dnl [  --enable-nsq           Enable nsq support])

if test "$PHP_NSQ" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-nsq -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/nsq.h"  # you most likely want to change this
  dnl if test -r $PHP_NSQ/$SEARCH_FOR; then # path given as parameter
  dnl   NSQ_DIR=$PHP_NSQ
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for nsq files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       NSQ_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$NSQ_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the nsq distribution])
  dnl fi

  dnl # --with-nsq -> add include path
  dnl PHP_ADD_INCLUDE($NSQ_DIR/include)

  dnl # --with-nsq -> check for lib and symbol presence
  dnl LIBNAME=nsq # you may want to change this
  dnl LIBSYMBOL=nsq # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $NSQ_DIR/$PHP_LIBDIR, NSQ_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_NSQLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong nsq lib version or lib not found])
  dnl ],[
  dnl   -L$NSQ_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(NSQ_SHARED_LIBADD)

  PHP_NEW_EXTENSION(nsq, nsq.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
