cmake_minimum_required(VERSION 3.17)
project(AtomicParsley)

find_package(ZLIB)
if(ZLIB_FOUND)
  include_directories(${ZLIB_INCLUDE_DIRS})
  add_definitions(-DHAVE_ZLIB_H)
endif()

list(APPEND sources
	src/CDtoc.cpp
	src/arrays.cpp
	src/compress.cpp
	src/extracts.cpp
	src/iconv.cpp
	src/id3v2.cpp
	src/main.cpp
	src/metalist.cpp
	src/parsley.cpp
	src/sha1.cpp
	src/util.cpp
	src/uuid.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  list(APPEND sources
    src/nsfile.mm
    src/nsimage.mm
  )
endif()

if(WIN32)
  list(APPEND sources
    src/extras/getopt.c
    src/extras/getopt1.c
  )
endif()

add_executable(
  AtomicParsley
  ${sources}
)

target_link_libraries(
  AtomicParsley
  ${ZLIB_LIBRARIES}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_link_libraries(
    AtomicParsley
    Cocoa
    Foundation
    IOKit
  )
endif()
