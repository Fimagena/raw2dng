ADD_SUBDIRECTORY(md5)
ADD_SUBDIRECTORY(xmp-sdk)
ADD_SUBDIRECTORY(dng-sdk)

FIND_PACKAGE(Threads)

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
                   )
ENDIF(WIN32)

# =======================================================
# libdng source code

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} 
					 ${CMAKE_CURRENT_SOURCE_DIR}/dng-sdk/source
				   )

# Add standalone C++ header files to this list
SET( LIBDNG_HDR ${CMAKE_CURRENT_SOURCE_DIR}/dnghost.h )

# Add library C++ source files to this list
SET( LIBDNG_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/dnghost.cpp )

# Library
ADD_LIBRARY( dng ${STATIC_FLAG} ${LIBDNG_SRCS} )

TARGET_LINK_LIBRARIES( dng dng-sdk ${CMAKE_THREAD_LIBS_INIT} )

# add the install targets
#INSTALL(TARGETS dng DESTINATION lib)
#INSTALL(FILES ${LIBDNG_HDR} DESTINATION include)

