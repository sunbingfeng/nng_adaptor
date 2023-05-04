#
# Copyright 2018 Capitar IT Group BV <info@capitar.com>
# Copyright 2018 Staysail Systems, Inc. <info@staysail.tech>
#
# This software is supplied under the terms of the MIT License, a
# copy of which should be located in the distribution where this
# file was obtained (LICENSE.txt).  A copy of the license may also be
# found online at https://opensource.org/licenses/MIT.

cmake_minimum_required (VERSION 2.8.12)

project(nng-adaptor)
add_definitions(-std=c++17)

file(GLOB_RECURSE ALL_LIBRARY_HDRS "include/*.h")
file(GLOB_RECURSE ALL_LIBRARY_SRCS "3rdparty/*.cpp")

set(ALL_SRCS ${ALL_LIBRARY_HDRS})
set(ALL_HDRS ${ALL_LIBRARY_SRCS})

find_package(Protobuf 3.0.0 REQUIRED)

# Call this from your own project's makefile.
find_package(nng CONFIG REQUIRED)
find_package(Threads)

add_library(nng_adaptor SHARED ${ALL_HDRS} ${ALL_SRCS})
target_include_directories(nng_adaptor PUBLIC include 3rdparty)
target_link_libraries(nng_adaptor ${PROTOBUF_LIBRARY})

file(GLOB_RECURSE ALL_PROTOS "proto/*.proto")
# TODO(cschuet): Move proto compilation to separate function.
set(ALL_PROTO_SRCS)
set(ALL_PROTO_HDRS)
foreach(ABS_FIL ${ALL_PROTOS})
  file(RELATIVE_PATH REL_FIL ${PROJECT_SOURCE_DIR} ${ABS_FIL})
  get_filename_component(DIR ${REL_FIL} DIRECTORY)
  get_filename_component(FIL_WE ${REL_FIL} NAME_WE)
  list(APPEND ALL_PROTO_SRCS "${PROJECT_BINARY_DIR}/${DIR}/${FIL_WE}.pb.cc")
  list(APPEND ALL_PROTO_HDRS "${PROJECT_BINARY_DIR}/${DIR}/${FIL_WE}.pb.h")

  add_custom_command(
    OUTPUT "${PROJECT_BINARY_DIR}/${DIR}/${FIL_WE}.pb.cc"
           "${PROJECT_BINARY_DIR}/${DIR}/${FIL_WE}.pb.h"
    COMMAND  ${PROTOBUF_PROTOC_EXECUTABLE}
    ARGS --cpp_out  ${PROJECT_BINARY_DIR} -I
      ${PROJECT_SOURCE_DIR} ${ABS_FIL}
    DEPENDS ${ABS_FIL}
    COMMENT "Running C++ protocol buffer compiler on ${ABS_FIL}"
    VERBATIM
  )
endforeach()
set_source_files_properties(${ALL_PROTO_SRCS} ${ALL_PROTO_HDRS} PROPERTIES GENERATED TRUE)
list(APPEND ALL_SRCS ${ALL_PROTO_SRCS})
list(APPEND ALL_HDRS ${ALL_PROTO_HDRS})
add_executable(demo demo.cpp ${ALL_SRCS} ${ALL_HDRS})
target_include_directories(demo PUBLIC include ${PROJECT_BINARY_DIR})
target_link_libraries(demo nng::nng ${PROTOBUF_LIBRARY} nng_adaptor)
