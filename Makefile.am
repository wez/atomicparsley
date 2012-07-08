
bin_PROGRAMS = AtomicParsley
noinst_HEADERS = src/AtomDefs.h src/AtomicParsley.h src/CDtoc.h \
  src/ap_types.h src/id3v2.h src/id3v2defs.h src/id3v2types.h \
  src/util.h

AM_CPPFLAGS = -DHAVE_CONFIG_H

if DARWIN
AM_CPPFLAGS += -DDARWIN_PLATFORM
AM_OBJCFLAGS = -DDARWIN_PLATFORM $(UNIVERSAL_FLAGS)
AM_CFLAGS = -DDARWIN_PLATFORM $(UNIVERSAL_FLAGS)
AM_CXXFLAGS = -DDARWIN_PLATFORM $(UNIVERSAL_FLAGS)
AM_LDFLAGS = $(UNIVERSAL_FLAGS) \
	-framework Cocoa -framework Foundation -framework IOKit
endif

AtomicParsley_SOURCES = \
	src/util.cpp \
	src/arrays.cpp \
	src/iconv.cpp \
	src/parsley.cpp \
	src/extracts.cpp \
	src/sha1.cpp \
	src/uuid.cpp \
	src/id3v2.cpp \
	src/metalist.cpp \
	src/CDtoc.cpp \
	src/compress.cpp \
	src/main.cpp 

# If you see a warning about this rule overriding an automake provided rule,
# that's fine; we do this because we want to build on versions of autoconf
# and automake prior to 1.12.1
.mm.o:
	$(OBJC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_OBJCFLAGS) $(OBJCFLAGS) -c -o $@ $<

if DARWIN
AtomicParsley_SOURCES += \
	src/nsfile.mm \
	src/nsimage.mm
endif

if NEED_GETOPT
AtomicParsley_SOURCES += \
	src/extras/getopt.c \
	src/extras/getopt1.c
endif

