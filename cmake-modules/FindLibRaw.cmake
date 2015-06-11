# - Try to find the LibRaw library
#
#  LIBRAW_MIN_VERSION - You can set this variable to the minimum version you need
#                      before doing FIND_PACKAGE(LibRaw). The default is 0.13.
#
# Once done this will define
#
#  LIBRAW_FOUND - system has LibRaw
#  LIBRAW_INCLUDE_DIR - the LibRaw include directory
#  LIBRAW_LIBRARIES - Link these to use LibRaw
#  LIBRAW_DEFINITIONS - Compiler switches required for using LibRaw
#
# The minimum required version of LibRaw can be specified using the
# standard syntax, e.g. find_package(LibRaw 0.13)
#
# For compatiblity, also the variable LIBRAW_MIN_VERSION can be set to the minimum version
# you need before doing FIND_PACKAGE(LibRaw). The default is 0.13.

# Copyright (c) 2010, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2008, Gilles Caulier, <caulier.gilles@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Support LIBRAW_MIN_VERSION for compatibility:
if(NOT LibRaw_FIND_VERSION)
  set(LibRaw_FIND_VERSION "${LIBRAW_MIN_VERSION}")
endif(NOT LibRaw_FIND_VERSION)

# the minimum version of LibRaw we require
if(NOT LibRaw_FIND_VERSION)
  set(LibRaw_FIND_VERSION "0.13")
endif(NOT LibRaw_FIND_VERSION)


if (NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   find_package(PkgConfig)
   pkg_check_modules(PC_LIBRAW QUIET libraw)
   set(LIBRAW_DEFINITIONS ${PC_LIBRAW_CFLAGS_OTHER})
endif (NOT WIN32)


find_path(LIBRAW_INCLUDE_DIR NAMES libraw/libraw.h
          HINTS
          ${PC_LIBRAW_INCLUDEDIR}
          ${PC_LIBRAW_INCLUDE_DIRS}
        )

find_library(LIBRAW_LIBRARY NAMES libraw raw
             HINTS
             ${PC_LIBRAW_LIBDIR}
             ${PC_LIBRAW_LIBRARY_DIRS}
            )


# Get the version number from libraw/libraw_version.h and store it in the cache:
if(LIBRAW_INCLUDE_DIR  AND NOT  LIBRAW_VERSION)
  file(READ ${LIBRAW_INCLUDE_DIR}/libraw/libraw_version.h LIBRAW_VERSION_CONTENT)
  string(REGEX MATCH "#define LIBRAW_MAJOR_VERSION +([0-9]+) *"  _dummy "${LIBRAW_VERSION_CONTENT}")
  set(LIBRAW_VERSION_MAJOR "${CMAKE_MATCH_1}")

  string(REGEX MATCH "#define LIBRAW_MINOR_VERSION +([0-9]+) *"  _dummy "${LIBRAW_VERSION_CONTENT}")
  set(LIBRAW_VERSION_MINOR "${CMAKE_MATCH_1}")

  string(REGEX MATCH "#define LIBRAW_PATCH_VERSION +([0-9]+) *"  _dummy "${LIBRAW_VERSION_CONTENT}")
  set(LIBRAW_VERSION_PATCH "${CMAKE_MATCH_1}")

  set(LIBRAW_VERSION "${LIBRAW_VERSION_MAJOR}.${LIBRAW_VERSION_MINOR}.${LIBRAW_VERSION_PATCH}" CACHE STRING "Version number of LibRaw" FORCE)
endif(LIBRAW_INCLUDE_DIR  AND NOT  LIBRAW_VERSION)

set(LIBRAW_LIBRARIES "${LIBRAW_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibRaw  REQUIRED_VARS  LIBRAW_LIBRARY LIBRAW_INCLUDE_DIR
                                         VERSION_VAR  LIBRAW_VERSION)

mark_as_advanced(LIBRAW_INCLUDE_DIR LIBRAW_LIBRARY)

