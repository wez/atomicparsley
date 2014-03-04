dnl vim:ts=2:sw=2:et:
dnl Process this file with autoconf to produce a configure script.

dnl Some intro checks and defines

AC_PREREQ(2.50)

AC_INIT([atomicparsley],[0.9.6],
  [http://bitbucket.org/wez/atomicparsley/issues/new/])
  
dnl Need to disable dependency tracking option so that the universal
dnl build option works for OSX
AM_INIT_AUTOMAKE([dist-bzip2 foreign no-dependencies subdir-objects -Wall])

AC_CANONICAL_HOST

AC_ARG_ENABLE(universal, dnl
[  --enable-universal     build a universal binary on Mac OS X [default=no]],
              universal=$enableval, universal=no)

case $host in
  *darwin*)

    AP_NATIVE_ARCH=`arch`
    AP_64_ARCH="x86_64"
    case $AP_NATIVE_ARCH in
      ppc*)
        AP_CROSS_ARCH="i386"
      ;;
    *)
        AP_CROSS_ARCH="ppc"
      ;;
    esac
    AC_SUBST(AP_CROSS_ARCH)
    AC_SUBST(AP_NATIVE_ARCH)
    AC_SUBST(AP_64_ARCH)

    HAVE_DARWIN_PLATFORM="true"
    AC_SUBST(HAVE_DARWIN_PLATFORM)

    AC_MSG_CHECKING([if we are os/x universal])
    if test "$universal" = "yes" ; then
      AC_MSG_RESULT([building for $AP_NATIVE_ARCH, $AP_CROSS_ARCH, and $AP_64_ARCH])
      UNIVERSAL_FLAGS="-arch $AP_NATIVE_ARCH -arch $AP_CROSS_ARCH -arch $AP_64_ARCH"
      AC_SUBST(UNIVERSAL_FLAGS)
    fi
    AC_MSG_RESULT([no])
    ;;
  *)
    universal="no"
    HAVE_DARWIN_PLATFORM="false"
    ;;
esac

AC_SUBST(universal)

AC_CONFIG_SRCDIR(src/parsley.cpp)
AC_CONFIG_HEADER(src/config.h)

AM_PROG_CC_C_O
AC_PROG_CXX
if test "$GCC" = "yes" ; then
  CFLAGS="$CFLAGS -Wall"
  CXXFLAGS="$CXXFLAGS -Wall"
fi
AC_PROG_OBJC
# Thanks automake; we MUST invoke AC_PROG_OBJCXX even though
# we roll our own objc++ build support, because automake 1.12.1
# throws a fatal error when it finds our .mm sources.
# We do this because there is no good reason to bump the minimum
# autoconf and automake dependencies.
m4_ifdef([AC_PROG_OBJCXX], [
AC_PROG_OBJCXX
])
AC_C_BIGENDIAN

AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([\
fcntl.h \
inttypes.h \
io.h \
getopt.h \
linux/cdrom.h \
math.h \
signal.h \
stddef.h \
stdint.h \
stdio.h \
stdlib.h \
string.h \
sys/ioctl.h \
sys/param.h \
sys/mount.h \
sys/stat.h \
sys/types.h \
sys/time.h \
time.h \
errno.h  \
zlib.h \
linux/cdrom.h \
unistd.h \
wchar.h \
windows.h \
])

AC_CHECK_HEADERS(getopt.h, [HAVE_GETOPT_H=1; AC_SUBST(HAVE_GETOPT_H)])

AC_C_CONST
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINTMAX_T
AC_TYPE_UINTPTR_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_INT16_T

AC_CHECK_FUNCS([\
fseeko \
fsetpos \
lroundf \
memset \
memcmp \
remove \
rename \
sranddev \
sscanf \
strdup \
strerror \
strftime \
strncmp \
strncasecmp \
strrchr \
strsep \
strstr \
strtol \
wmemset \
])

AC_FUNC_MALLOC

AC_CHECK_LIB(z, deflateEnd, [
  HAVE_LIBZ=1
  AC_SUBST(HAVE_LIBZ)
  LIBS="$LIBS -lz"
],[
  AC_MSG_ERROR([zlib is required])
]
)

CFLAGS="-D_FILE_OFFSET_BITS=64 $CFLAGS"
CXXFLAGS="-D_FILE_OFFSET_BITS=64 $CXXFLAGS"

AC_ARG_ENABLE(debug, dnl
[  --disable-debug         do not build a debug version [default=yes]],
              debug=$enableval, debug=no)
if test "$debug" = "yes" ; then
  AC_DEFINE_UNQUOTED(DEBUG, $debug, [build binary with debug output])
  AC_SUBST(debug)
fi

AM_CONDITIONAL([NEED_GETOPT], [test x$HAVE_GETOPT_H = x])
AM_CONDITIONAL([DARWIN], [test x$HAVE_DARWIN_PLATFORM = xtrue])
AM_CONDITIONAL([UNIVERSAL], [test x$universal = xyes])

AC_OUTPUT([Makefile])

echo "+----------------------------------------------+"
echo "|                  SUCCESS                     |"
echo "+----------------------------------------------+"
echo "  AtomicParsley has been configured, you should"
echo "  now type 'make' to compile AtomicParsley."
echo
echo "+----------------------------------------------+"
echo "|             YOUR CONFIGURATION               |"
echo "+----------------------------------------------+"

echo "  Version:           $PACKAGE_VERSION"

if test "$universal" = "yes" ; then
	echo "  universal build:   enabled"
fi	

if test "$debug" = "no" ; then
	echo "  debug build:       disabled"
else
	echo "  debug build:       enabled"
fi

echo
