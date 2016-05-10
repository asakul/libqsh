
cmake_minimum_required(VERSION 3.0)

project(libqsh)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(WORKSPACE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

include_directories(${CMAKE_SOURCE_DIR}/3rdparty)

set(CMAKE_CXX_FLAGS  "-Wall -fno-strict-aliasing -fexceptions -g --std=gnu++14 -mtune=generic ${PLATFORM_CXX_FLAGS} -fno-omit-frame-pointer")

set(libqsh-headers
	include/qsh/types.h
	)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(test-sources
	tests/test.cpp
	
	tests/testqshfile.cpp
	)

add_executable(libqsh-test ${test-sources})
target_link_libraries(libqsh-test)

