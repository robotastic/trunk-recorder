# Copyright 2011 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

########################################################################
# Project setup
########################################################################

cmake_minimum_required(VERSION 2.6)
project(Trunk-Recorder CXX C)

set(CMAKE_BUILD_TYPE Debug)

#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

########################################################################
# Compiler specific setup
########################################################################
if(CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32)
    #http://gcc.gnu.org/wiki/Visibility
    add_definitions(-fvisibility=hidden)
endif()
if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
endif()

########################################################################
# Install directories
########################################################################
include(GrPlatform) #define LIB_SUFFIX
set(GR_RUNTIME_DIR      bin)
set(GR_LIBRARY_DIR      lib${LIB_SUFFIX})
set(GR_INCLUDE_DIR      include/howto)
set(GR_DATA_DIR         share)
set(GR_PKG_DATA_DIR     ${GR_DATA_DIR}/${CMAKE_PROJECT_NAME})
set(GR_DOC_DIR          ${GR_DATA_DIR}/doc)
set(GR_PKG_DOC_DIR      ${GR_DOC_DIR}/${CMAKE_PROJECT_NAME})
set(GR_CONF_DIR         etc)
set(GR_PKG_CONF_DIR     ${GR_CONF_DIR}/${CMAKE_PROJECT_NAME}/conf.d)
set(GR_LIBEXEC_DIR      libexec)
set(GR_PKG_LIBEXEC_DIR  ${GR_LIBEXEC_DIR}/${CMAKE_PROJECT_NAME})
set(GRC_BLOCKS_DIR      ${GR_PKG_DATA_DIR}/grc/blocks)


########################################################################
# Find gnuradio build dependencies
########################################################################
find_package(GnuradioRuntime)
find_package(CppUnit)

# To run a more advanced search for GNU Radio and it's components and
# versions, use the following. Add any components required to the list
# of GR_REQUIRED_COMPONENTS (in all caps) and change "version" to the
# minimum API compatible version required.
#
# set(GR_REQUIRED_COMPONENTS RUNTIME BLOCKS FILTER ...)
# find_package(Gnuradio "version")
#if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
#    set(GR_REQUIRED_COMPONENTS ANALOG DIGITAL RUNTIME BLOCKS FILTER PMT)
#    find_package(Gnuradio)
#endif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")

set(GR_REQUIRED_COMPONENTS ANALOG DIGITAL RUNTIME BLOCKS FILTER PMT)
find_package(Gnuradio)
find_package(GnuradioRuntime)
find_package(CppUnit)

if(NOT GNURADIO_RUNTIME_FOUND)
    message(FATAL_ERROR "GnuRadio Runtime required to compile Trunk Recorder")
endif()
if(NOT CPPUNIT_FOUND)
    message(FATAL_ERROR "CppUnit required to compile Trunk Recorder")
endif()



########################################################################
# Find build dependencies
########################################################################
#find_package(LibOsmoSDR)
#find_package(LibRTLSDR)
#find_package(LibMiriSDR)
#find_package(LibbladeRF)
find_package(GnuradioUHD)
find_package(GrOsmoSDR)
find_package(LibHackRF)
find_package(UHD)
find_package(OpenSSL REQUIRED)

if(NOT GNURADIO_RUNTIME_FOUND)
    message(FATAL_ERROR "GnuRadio Runtime required to build " ${CMAKE_PROJECT_NAME})
endif()

########################################################################
# Setup boost
########################################################################
MESSAGE(STATUS "Configuring Boost C++ Libraries...")

# Although not required on my system, some users have linking issues without
SET(BOOST_REQUIRED_COMPONENTS
    thread
    system
    program_options
    filesystem
    log_setup
    log
    regex
    random
)

if(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")
    list(APPEND BOOST_LIBRARYDIR "/usr/lib64") #fedora 64-bit fix
endif(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")

set(Boost_ADDITIONAL_VERSIONS
    "1.35.0" "1.35" "1.36.0" "1.36" "1.37.0" "1.37" "1.38.0" "1.38" "1.39.0" "1.39"
    "1.40.0" "1.40" "1.41.0" "1.41" "1.42.0" "1.42" "1.43.0" "1.43" "1.44.0" "1.44"
    "1.45.0" "1.45" "1.46.0" "1.46" "1.47.0" "1.47" "1.48.0" "1.48" "1.49.0" "1.49"
    "1.50.0" "1.50" "1.51.0" "1.51" "1.52.0" "1.52" "1.53.0" "1.53" "1.54.0" "1.54"
    "1.55.0" "1.55" "1.56.0" "1.56" "1.57.0" "1.57" "1.58.0" "1.58" "1.59.0" "1.59"
    "1.60.0" "1.60" "1.61.0" "1.61" "1.62.0" "1.62" "1.63.0" "1.63" "1.64.0" "1.64"
    "1.65.0" "1.65" "1.66.0" "1.66" "1.67.0" "1.67" "1.68.0" "1.68" "1.69.0" "1.69"
)

find_package(Boost COMPONENTS ${BOOST_REQUIRED_COMPONENTS})

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost required to build " ${CMAKE_PROJECT_NAME})
endif()

ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK)


########################################################################
# Setup the include and linker paths
########################################################################
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${GNURADIO_RUNTIME_INCLUDE_DIRS}
    ${GROSMOSDR_INCLUDE_DIR}
    ${CMAKE_BINARY_DIR}/lib
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/op25_repeater/lib
    ${CMAKE_SOURCE_DIR}/lib/op25_repeater/include
    ${OPENSSL_INCLUDE_DIR}
    "lib"
)

link_directories(
    ${Boost_LIBRARY_DIRS}
    ${GNURADIO_RUNTIME_LIBRARY_DIRS}
    ${GROSMOSDR_LIBRARIES_DIRS}
    ${GNURADIO_UHD_LIBRARIES_DIRS}
    ${UHD_LIBRARIES_DIRS}
    ${OPENSSL_ROOT_DIR}/lib
)

#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wno-unused-local-typedef -Wno-deprecated-declarations -Wno-error=deprecated-declarations -g")



#include(CheckCXXCompilerFlag)
#CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
#CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
#if(COMPILER_SUPPORTS_CXX11)
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#elseif(COMPILER_SUPPORTS_CXX0X)
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
#else()
#        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
#endif()

#SET(CMAKE_CXX_STANDARD 98)
SET(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wno-narrowing")


add_subdirectory(lib/op25_repeater)


list(APPEND trunk_recorder_sources
  trunk-recorder/main.cc
  trunk-recorder/formatter.cc
  trunk-recorder/source.cc
  trunk-recorder/uploaders/uploader.cc
  trunk-recorder/uploaders/call_uploader.cc
  trunk-recorder/uploaders/stat_socket.cc
  trunk-recorder/call_conventional.cc
  trunk-recorder/call.cc
  trunk-recorder/systems/smartnet_trunking.cc
  trunk-recorder/systems/p25_trunking.cc
  trunk-recorder/systems/smartnet_parser.cc
  trunk-recorder/systems/p25_parser.cc
  trunk-recorder/systems/smartnet_decode.cc
  trunk-recorder/systems/system.cc
  trunk-recorder/recorders/recorder.cc
  trunk-recorder/recorders/debug_recorder.cc
  trunk-recorder/recorders/sigmf_recorder.cc
  trunk-recorder/recorders/analog_recorder.cc
  lib/gr_blocks/nonstop_wavfile_sink_impl.cc
  lib/gr_blocks/nonstop_wavfile_delayopen_sink_impl.cc
  trunk-recorder/recorders/p25conventional_recorder.cc
  trunk-recorder/recorders/p25_recorder.cc
  trunk-recorder/talkgroup.cc
  trunk-recorder/talkgroups.cc
  lib/gr_blocks/freq_xlating_fft_filter.cc
  lib/lfsr/lfsr.cxx
  )

set_source_files_properties(lib/lfsr/lfsr.cxx COMPILE_FLAGS "-w")

target_compile_options(
    imbe_vocoder PRIVATE
    "-w"
)

target_compile_options(
    gnuradio-op25_repeater PRIVATE
    "-w"
)

add_executable(recorder ${trunk_recorder_sources})

target_link_libraries(recorder ssl crypto ${GNURADIO_PMT_LIBRARIES} ${GNURADIO_RUNTIME_LIBRARIES} ${GNURADIO_FILTER_LIBRARIES} ${GNURADIO_DIGITAL_LIBRARIES} ${GNURADIO_ANALOG_LIBRARIES} ${GNURADIO_AUDIO_LIBRARIES} ${GNURADIO_UHD_LIBRARIES} ${UHD_LIBRARIES} ${GNURADIO_BLOCKS_LIBRARIES} ${GROSMOSDR_LIBRARIES} ${Boost_LIBRARIES} ${LIBOP25_REPEATER_LIBRARIES} gnuradio-op25_repeater imbe_vocoder)
