cmake_minimum_required(VERSION 3.0)
project(protocute)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(protocute main.cpp proto.hpp proto.cpp)

set(Boost_USE_STATIC_LIBS ON)
add_definitions(-DBOOST_BIND_NO_PLACEHOLDERS=1 -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE=1) # boost::_1 conflicts with std::_1
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY=1 -DBOOST_SYSTEM_NO_DEPRECATED=1) # required for header-only compilation
add_definitions(-DBOOST_DATE_TIME_NO_LIB=1 -DBOOST_SYSTEM_NO_LIB=1 -DBOOST_REGEX_NO_LIB=1) # required for header-only compilation
set(Boost_USE_MULTITHREADED OFF) # all boost libraries are multithreaded since some version
find_package(Boost 1.65)
if(Boost_FOUND)
    message( STATUS "Boost found by find_boost, Boost_INCLUDE_DIRS: " ${Boost_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Boost not found, please download and unpack boost into ${PARENT_DIR}/boost")
endif()
include_directories(${Boost_INCLUDE_DIRS})

