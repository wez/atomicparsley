#!/bin/sh
# vim:ts=2:sw=2:et:
autoheader
aclocal
automake --add-missing --foreign
autoconf
