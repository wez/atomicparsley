# AtomicParsley

![Maintenance](https://img.shields.io/maintenance/no/2012)

**Maintenance Note:** This repo is not actively maintained by @wez.

A note from @wez:

> I made some fixes in 2009 and became the de-facto fork of AtomicParsley as a
> result.  However, I haven't used this tool myself in many years and have
> acted in a very loose guiding role since then.
>
> In 2020 Bitbucket decided to cease hosting Mercurial based repositories
> which meant that I had to move it in order to keep it alive, so you'll
> see a flurry of recent activity.
>
> I'll consider merging pull requests if they are easy to review, but because
> I don't use this tool myself I have no way to verify complex changes.
> If you'd like to make such a change, please consider contributing some
> kind of basic automated test with a corresponding small test file.
> This repo does have GitHub Actions enabled so bootstrapping some test
> coverage is now feasible.

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

To *not* build a Mac OS X universal binary:

      % make maintainer-clean
      % ./configure --disable-universal
      % make


## For Windows users:
AtomicParsley builds under cygwin and/or mingw using the same procedure as above.

Foosatraz built in Windows 8 using MinGW.

MinGW-get version 0.6.2-beta-20131004-1

mingw-libz version 1.2.8-1 from MinGW Installation Manager

      % ./autogen.sh
      % ./configure --prefix=/mingw
      % make LDFLAGS=-static
      % strip AtomicParsley.exe

Full details [pdf](https://bitbucket.org/Foosatraz/wez-atomicparsley-foosatraz-fork/downloads/AtomicParsleyMinGWBuildNotebook.pdf)

To build with MSVC, you will need to create your own project file; look
at the list of source files in Makefile.am; you need to add all of the
source files *except* the .mm files.  You will also need to provide your
own zlib.

If you don't want to build it yourself, [Jon Hedgrows' fork ](https://bitbucket.org/jonhedgerows/atomicparsley/wiki/Home) maintains pre-built Windows binaries of the Wez fork:
[Windows Downloads](https://bitbucket.org/jonhedgerows/atomicparsley/downloads)

