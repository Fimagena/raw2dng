# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# The version number.
SET(RAW2DNG_VERSION_MAJOR 1)
SET(RAW2DNG_VERSION_MINOR 2)
SET(RAW2DNG_VERSION_PATCH 0)

# configure a header file to pass some of the CMake settings
# to the source code
CONFIGURE_FILE (
  "${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
  )

SET( EXIV2_MIN_VERSION "0.21" )
FIND_PACKAGE( Exiv2 )
INCLUDE_DIRECTORIES( ${EXIV2_INCLUDE_DIR} )
ADD_DEFINITIONS(${EXIV2_DEFINITIONS})

FIND_PACKAGE( LibRaw )
INCLUDE_DIRECTORIES( ${LIBRAW_INCLUDE_DIR} )
ADD_DEFINITIONS(${LIBRAW_DEFINITIONS})

FIND_PACKAGE( ZLIB )
INCLUDE_DIRECTORIES( ${ZLIB_INCLUDE_DIR} )
ADD_DEFINITIONS(${ZLIB_DEFINITIONS})

INCLUDE_DIRECTORIES( ${raw2dng_SOURCE_DIR}/libdng 
                     ${raw2dng_SOURCE_DIR}/libdng/dng-sdk/source 
                     ${PROJECT_BINARY_DIR} )

# Set platforms env.
IF(WIN32)
    ADD_DEFINITIONS(
                    # Set Windows rules.
                    -DqWinOS=1
                    -DqMacOS=0
                   )
ELSE(WIN32)
    ADD_DEFINITIONS(
                    # Set MACOS-X, Linux & co rules.
                    -DqWinOS=0
                    -DqMacOS=0

                    # Must be set to 1, else do not compile under Linux.
                    -DqDNGUseStdInt=1
                   )
ENDIF(WIN32)

# Check processor endianness
INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
IF(NOT IS_BIG_ENDIAN)
    ADD_DEFINITIONS(-DqDNGLittleEndian=1)
ENDIF(NOT IS_BIG_ENDIAN)

# Thread safe support under Mac and Linux using pthread library
IF(NOT WIN32)
    ADD_DEFINITIONS(-DqDNGThreadSafe=1)
ENDIF(NOT WIN32)

SET( RAW2DNG_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/raw2dng.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/negativeProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rawConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vendorProcessors/ILCE7processor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vendorProcessors/FujiProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vendorProcessors/variousVendorProcessor.cpp
   )

# Level of debug info in the console.
ADD_DEFINITIONS(-DqDNGDebug=0)
ADD_DEFINITIONS(-DqDNGValidateTarget=1)

ADD_EXECUTABLE( raw2dng ${RAW2DNG_SRCS} )

TARGET_LINK_LIBRARIES(raw2dng dng
                              ${ZLIB_LIBRARIES}
                              ${LIBRAW_LIBRARIES}
                              ${EXIV2_LIBRARIES})

ADD_EXECUTABLE( test ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp)
TARGET_LINK_LIBRARIES (test ${EXIV2_LIBRARIES})

# add the install targets
INSTALL(TARGETS raw2dng DESTINATION bin)
