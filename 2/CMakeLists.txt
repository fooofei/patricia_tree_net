
cmake_minimum_required(VERSION 3.1)
project(C_MAKE_DEMO)

if(APPLE)
    # no warning
    # https://stackoverflow.com/questions/31561309/cmake-warnings-under-os-x-macosx-rpath-is-not-specified-for-the-following-targe
    set(CMAKE_MACOSX_RPATH 0)
endif()

set(BINARY_NAME mtest)

set(Source_files 
    ${CMAKE_CURRENT_LIST_DIR}/patricia.c
    ${CMAKE_CURRENT_LIST_DIR}/patricia.h
    ${CMAKE_CURRENT_LIST_DIR}/demo.c
    ${CMAKE_CURRENT_LIST_DIR}/../ipaddress.c
    ${CMAKE_CURRENT_LIST_DIR}/../ipaddress.h
    ${CMAKE_CURRENT_LIST_DIR}/crt_dbg_leak.h
    ${CMAKE_CURRENT_LIST_DIR}/crt_dbg_leak.c
	)

add_executable(${BINARY_NAME} ${Source_files})

source_group(files FILES ${Source_files})

if(WIN32)
	target_compile_options(${BINARY_NAME} PRIVATE /source-charset:utf-8 /execution-charset:utf-8)
endif()
