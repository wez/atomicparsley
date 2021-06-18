cmake_minimum_required(VERSION 3.17)
project(AtomicParsley)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(ASAN "whether to enable ASAN" OFF)

find_program(GIT git)
if(GIT)
  execute_process(
    COMMAND "${GIT}" "show" "-s" "--format=%H;%cd" "--date=format:%Y%m%d.%H%M%S.0"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE git_result
    OUTPUT_VARIABLE git_data
    ERROR_VARIABLE git_err
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(git_result EQUAL 0)
    list(GET git_data 0 BUILD_INFO)
    list(GET git_data 1 PACKAGE_VERSION)
  endif()
endif()

include(CheckSymbolExists)
check_symbol_exists(strsep "string.h" HAVE_STRSEP)
if(HAVE_STRSEP)
  add_definitions(-DHAVE_STRSEP)
endif()
check_symbol_exists(fseeko "stdio.h" HAVE_FSEEKO)
if(HAVE_FSEEKO)
  add_definitions(-DHAVE_FSEEKO)
endif()

add_definitions(
  -DPACKAGE_VERSION="${PACKAGE_VERSION}"
  -DBUILD_INFO="${BUILD_INFO}"
  -D_FILE_OFFSET_BITS=64
)

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

if(ZLIB_FOUND)
  target_link_libraries(
    AtomicParsley
    ${ZLIB_LIBRARIES}
  )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_link_libraries(
    AtomicParsley
    "-framework Cocoa"
    "-framework Foundation"
    "-framework IOKit"
  )
endif()

if (ASAN)
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
  set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif()

install(TARGETS AtomicParsley RUNTIME)
