cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(PROJECT_NAME "TS-PARSER")
project(${PROJECT_NAME})

list(APPEND CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows")
find_package(mpg123 CONFIG REQUIRED)

# force static runtime libraries for msvc builds
if(MSVC)
  set(variables CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_MINSIZEREL)
  foreach(variable ${variables})
  if(${variable} MATCHES "/MD")
    string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
  endif()
  endforeach()
endif()

# set c++17
set (CMAKE_CXX_STANDARD 17)
set( CMAKE_CXX_STANDARD_REQUIRED ON )

# compile everything position independent (even static libraries)
set( CMAKE_POSITION_INDEPENDENT_CODE TRUE )

# set verbose compile options
#set( CMAKE_VERBOSE_MAKEFILE ON )

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /PROFILE")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

set(PROJECT_SOURCES  
  tsCommon.h
  tsTS.h
  tsTransportStreamHeader.h tsTransportStreamHeader.cpp
  tsTransportStreamAF.h tsTransportStreamAF.cpp
  tsPESPacketHeader.h tsPESPacketHeader.cpp
  tsPESAssembler.h tsPESAssembler.cpp
  tsMPEGAudioHeader.h tsMPEGAudioHeader.cpp
  tsWaveWriter.h tsWaveWriter.cpp
  tsMPG123Decoder.h tsMPG123Decoder.cpp
  TS_parser.cpp)

source_group("Source Files" FILES ${PROJECT_SOURCES})

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE MPG123::libmpg123)


