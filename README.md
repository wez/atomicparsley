# AtomicParsley

## Basic Instructions

If you are building from source you will need autoconf & automake (you will
definitely need make):

      % ./autogen.sh
      % ./configure
      % make

Use the program in situ or place it somewhere in your $PATH by using:

      % sudo make install


### Dependencies:

zlib  - used to compress ID3 frames & expand already compressed frames
        available from http://www.zlib.net

## For Mac OS X users:

The default is to build a universal binary.

switching between a universal build and a platform dependent build should be
accompanied by a "make maintainter-clean" and a ./configure between builds.

To *not* building a Mac OS X universal binary:

      % make maintainer-clean
      % ./configure --disable-universal
      % make


## For Windows users:

AtomicParsley builds under cygwin and/or mingw using the same procedure
as above.

To build with MSVC, you will need to create your own project file; look
at the list of source files in Makefile.am; you need to add all of the
source files *except* the .mm files.  You will also need to provide your
own zlib.

If you don't want to build it yourself, Jon Hedgrows' forks maintains
pre-built Windows binaries:

[Windows Downloads](https://bitbucket.org/jonhedgerows/atomicparsley/downloads)

<!-- vim:ts=2:sw=2:et:
-->
