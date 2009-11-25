Basic Instructions

If you are building from svn source you will need autoconf & automake (you will definitely need make), otherwise skip to step 2.
   
   1. Create the configure script:
      % autoconf && autoheader

   2. Run the configure script:
      % ./configure

      You can check the meager options with ./configure -h

   3. Build the program
      % make

   4. Use the program in situ or place it somewhere in your $PATH

Dependencies:
zlib  - used to compress ID3 frames & expand already compressed frames
        available from http://www.zlib.net

--------
Notes:
For Mac OS X users:
switching between a universal build and a platform dependent build should be accompanied by a "make maint-clean" and a ./configure between builds.

To build a Mac OS X universal binary:
% make maint-clean
% ./configure --enable-universal
# make