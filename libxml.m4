# Configure paths for libxml
# By Greg Banks <gnb@alphalink.com.au>
# Copied and changed from gtk.m4, which bore the message:
# Owen Taylor     97-11-3

dnl AM_PATH_LIBXML([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]]])
dnl Test for libxml, and define LIBXML_CFLAGS and LIBXML_LIBS
dnl
AC_DEFUN(AM_PATH_LIBXML,
[dnl 
dnl Get the cflags and libraries from the xml-config script
dnl
AC_ARG_WITH(libxml-prefix,[  --with-libxml-prefix=PFX Prefix where libxml is installed (optional)],
            libxml_config_prefix="$withval", libxml_config_prefix="")
AC_ARG_WITH(libxml-exec-prefix,[  --with-libxml-exec-prefix=PFX Exec prefix where libxml is installed (optional)],
            libxml_config_exec_prefix="$withval", libxml_config_exec_prefix="")

  if test x$libxml_config_exec_prefix != x ; then
     libxml_config_args="$libxml_config_args --exec-prefix=$libxml_config_exec_prefix"
     if test x${LIBXML_CONFIG+set} != xset ; then
        LIBXML_CONFIG=$libxml_config_exec_prefix/bin/xml-config
     fi
  fi
  if test x$libxml_config_prefix != x ; then
     libxml_config_args="$libxml_config_args --prefix=$libxml_config_prefix"
     if test x${LIBXML_CONFIG+set} != xset ; then
        LIBXML_CONFIG=$gtk_config_prefix/bin/xml-config
     fi
  fi

  AC_PATH_PROG(LIBXML_CONFIG, xml-config, no)
  min_libxml_version=ifelse([$1], ,1.8.7,$1)
  AC_MSG_CHECKING(for libxml - version >= $min_libxml_version)
  no_libxml=""
  
ac_xml_check_versions ()
{
    # actual version
    v1_major=`echo [$]1 | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    v1_minor=`echo [$]1 | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    v1_micro=`echo [$]1 | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    # required version
    v2_major=`echo [$]2 | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    v2_minor=`echo [$]2 | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    v2_micro=`echo [$]2 | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    
    if test $v1_major -gt $v2_minor ; then
    	echo yes
    elif test $v1_minor -gt $v2_minor ; then
    	echo yes
    elif test $v1_micro -ge $v2_micro ; then
    	echo yes
    else
    	echo no
    fi
}
  
  if test "$LIBXML_CONFIG" = "no" ; then
    no_libxml=yes
  else
    LIBXML_CFLAGS=`$LIBXML_CONFIG $libxml_config_args --cflags`
    LIBXML_LIBS=`$LIBXML_CONFIG $libxml_config_args --libs`
    
    dnl
    dnl Unlike gtk.m4, there's no real benefit to compiling and running
    dnl a test program, because libxml doesn't provide version macros
    dnl in the header to match against compiled version numbers in the
    dnl shared library.  So we trust that if we can find the xml-config
    dnl program then libxml is installed correctly, and just check the
    dnl version numbers in some funky shell code  -- Greg Banks.
    dnl

    libxml_version=`$LIBXML_CONFIG $libxml_config_args --version`
    if test `ac_xml_check_versions "$libxml_version" "$min_libxml_version"` = "no" ; then
    	no_libxml=yes
    fi
  fi

  if test "x$no_libxml" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$LIBXML_CONFIG" = "no" ; then
       echo "*** The xml-config script installed by libxml could not be found"
       echo "*** If libxml was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the LIBXML_CONFIG environment variable to the"
       echo "*** full path to xml-config."
     fi
     LIBXML_CFLAGS=""
     LIBXML_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(LIBXML_CFLAGS)
  AC_SUBST(LIBXML_LIBS)
])
