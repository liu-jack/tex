dnl
dnl $ Id: $
dnl

PHP_ARG_ENABLE(sdpphp, whether to enable sdpphp functions,
[  --enable-sdpphp         Enable sdpphp support])

if test "$PHP_SDPPHP" != "no"; then
  export OLD_CPPFLAGS="$CPPFLAGS"
  export CPPFLAGS="$CPPFLAGS $INCLUDES -DHAVE_SDPPHP"

  AC_MSG_CHECKING(PHP version)
  AC_TRY_COMPILE([#include <php_version.h>], [
#if PHP_VERSION_ID < 40000
#error  this extension requires at least PHP version 4.0.0
#endif
],
[AC_MSG_RESULT(ok)],
[AC_MSG_ERROR([need at least PHP 4.0.0])])

  export CPPFLAGS="$OLD_CPPFLAGS"


  PHP_SUBST(SDPPHP_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(stdc++, "", SDPPHP_SHARED_LIBADD)
  AC_DEFINE(HAVE_SDPPHP, 1, [ ])

  PHP_NEW_EXTENSION(sdpphp, sdpphp.cpp sdpphp_imp.cpp, $ext_shared)

fi

