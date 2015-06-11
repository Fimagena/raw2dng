# =======================================================
# MD5 source code.

SET( LIBMD5_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/MD5.cpp)

ADD_LIBRARY( md5 STATIC ${LIBMD5_SRCS} )

TARGET_LINK_LIBRARIES( md5 )
