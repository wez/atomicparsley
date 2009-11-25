dnl aclocal.m4: macros autoconf uses when building configure from configure.ac
dnl
dnl Copyright (C) 2006 puck_lock
dnl
dnl AtomicParsley is GPL software; you can freely distribute,
dnl redistribute, modify & use under the terms of the GNU General
dnl Public License; either version 2 or its successor.
dnl
dnl AtomicParsley is distributed under the GPL "AS IS", without
dnl any warranty; without the implied warranty of merchantability
dnl or fitness for either an expressed or implied particular purpose.
dnl
dnl Please see the included GNU General Public License (GPL) for 
dnl your rights and further details; see the file COPYING. If you
dnl cannot, write to the Free Software Foundation, 59 Temple Place
dnl Suite 330, Boston, MA 02111-1307, USA.  Or www.fsf.org
dnl

dnl
dnl Checks for OS.
dnl

dnl AP_OS_VERSION()
dnl
AC_DEFUN([AC_OS_VERSION],
[
  AP_OS_NAME=`uname -s | cut -d '_' -f1`
  AP_OS_RELVER=`uname -r`
  AP_NATIVE_ARCH=`arch`
  AC_SUBST(AP_NATIVE_ARCH)
  if [ test $AP_NATIVE_ARCH="ppc" || test $AP_NATIVE_ARCH="ppc64" ]; then
    AP_CROSS_ARCH="i386"
    AC_SUBST(AP_CROSS_ARCH)
  else
    AP_CROSS_ARCH="ppc"
    AC_SUBST(AP_CROSS_ARCH)
  fi
])

dnl AP_DARWIN_CHECK_UNIVERSAL_SDK
dnl
dnl Check for Mac OS X 10.4 Univeral SDK
dnl
AC_DEFUN([AC_CHECK_DARWIN_UNIVERSAL_SDK],
[
  if test $AP_OS_NAME="Darwin"; then
    AC_MSG_CHECKING([for Mac OS X 10.4 Universal SDK])
    if test -r /Developer/SDKs/MacOSX10.4u.sdk/; then
      AC_MSG_RESULT([yes])
      DARWIN_U_SYSROOT="/Developer/SDKs/MacOSX10.4u.sdk"
      AC_SUBST(DARWIN_U_SYSROOT)
    else
      AC_MSG_RESULT([no])
    fi
  fi
])
