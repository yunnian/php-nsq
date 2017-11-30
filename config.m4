dnl $Id$
dnl config.m4 for extension nsq

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(nsq, for nsq support,
 Make sure that the comment is aligned:
 [  --with-nsq             Include nsq support])

PHP_ARG_WITH(libevent-path, for libevent support,
 Make sure that the comment is aligned:
 [  --with-libevent-path[=DIR]             you libevent path],no,no)

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(nsq, whether to enable nsq support,
dnl Make sure that the comment is aligned:
dnl [  --enable-nsq           Enable nsq support])

if test "$PHP_NSQ" != "no"; then
  SEARCH_FOR="/include/event2/event.h"
  if test "$PHP_LIBEVENT_PATH" != "no"; then
    AC_MSG_CHECKING([for libevent headers in $PHP_LIBEVENT_PATH])
    if test -r $PHP_LIBEVENT_PATH/$SEARCH_FOR; then
      LIBEVENT_DIR=$PHP_LIBEVENT_PATH
      AC_MSG_RESULT([found])
    fi
  else
    AC_MSG_CHECKING([for libevent headers in default path])
    SEARCH_PATH="/usr /usr/local /opt/local /usr/local/Cellar/libevent/*"
    for i in $SEARCH_PATH ; do
      if test -r $i/$SEARCH_FOR; then
        LIBEVENT_DIR=$i
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi

echo "libevent-path:$LIBEVENT_DIR";
  if test -z "$LIBEVENT_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Cannot find libevent headers])
  fi

  PHP_ADD_INCLUDE($LIBEVENT_DIR/include)

  LIBNAME=event
  LIBSYMBOL=event_base_new

  if test "x$PHP_LIBDIR" = "x"; then
    PHP_LIBDIR=lib
  fi

  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
    PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $LIBEVENT_DIR/$PHP_LIBDIR, LIBEVENT_SHARED_LIBADD)
  ],[
    AC_MSG_ERROR([wrong libevent version {1.4.+ is required} or lib not found])
  ],[
    -L$LIBEVENT_DIR/$PHP_LIBDIR 
  ])

  PHP_ADD_EXTENSION_DEP(libevent, sockets, true)
  PHP_SUBST(LIBEVENT_SHARED_LIBADD)
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
