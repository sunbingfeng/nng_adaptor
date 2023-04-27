#
# Copyright 2018 Capitar IT Group BV <info@capitar.com>
# Copyright 2018 Staysail Systems, Inc. <info@staysail.tech>
#
# This software is supplied under the terms of the MIT License, a
# copy of which should be located in the distribution where this
# file was obtained (LICENSE.txt).  A copy of the license may also be
# found online at https://opensource.org/licenses/MIT.

cmake_minimum_required (VERSION 2.8.12)

project(nng-adaptor-demo)
add_definitions(-std=c++17)

# Call this from your own project's makefile.
find_package(nng CONFIG REQUIRED)
find_package(Threads)

add_executable(demo demo.cpp)
target_include_directories(demo PUBLIC include)
target_link_libraries(demo nng::nng)
