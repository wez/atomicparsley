Basic Instructions

If you are building from source you will need autoconf & automake (you will
definitely need make), otherwise skip to step 2.
   
   1. Create the configure script:
	  % ./autogen.sh

   2. Run the configure script:
      % ./configure

      You can check the meager options with ./configure -h

   3. Build the program
      % make

   4. Use the program in situ or place it somewhere in your $PATH

Dependencies:

zlib  - used to compress ID3 frames & expand already compressed frames
        available from http://www.zlib.net


For Mac OS X users:

The default is to build a universal binary.

switching between a universal build and a platform dependent build should be
accompanied by a "make maint-clean" and a ./configure between builds.

To *not( build a Mac OS X universal binary:

  % make maint-clean
  % ./configure --disable-universal
  % make


For Windows users:

AtomicParsley builds under cygwin using the same procedure as above.

To build with MSVC, you will need to create your own project file; look at the
list of source files in Makefile.am; you need all of the source files *except*
the .mm files.  You will also need to provide your own zlib.


# vim:ts=2:sw=2:et:
