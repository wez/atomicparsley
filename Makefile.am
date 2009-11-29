
bin_PROGRAMS = AtomicParsley

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
	src/AP_commons.cpp \
	src/AP_arrays.cpp \
	src/AP_iconv.cpp \
	src/AtomicParsley.cpp \
	src/AP_AtomExtracts.cpp \
	src/APar_sha1.cpp \
	src/APar_uuid.cpp \
	src/AP_ID3v2_tags.cpp \
	src/AP_MetadataListings.cpp \
	src/AP_CDTOC.cpp \
	src/APar_zlib.cpp \
	src/main.cpp 

.mm.o:
	$(OBJC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_OBJCFLAGS) $(OBJCFLAGS) -c -o $@ $<

if DARWIN
AtomicParsley_SOURCES += \
	src/AP_NSFile_utils.mm \
	src/AP_NSImage.mm
endif

if NEED_GETOPT
AtomicParsley_SOURCES += \
	src/extras/getopt.c \
	src/extras/getopt1.c
endif

