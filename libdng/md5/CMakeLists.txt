# =======================================================
# MD5 source code.

ADD_LIBRARY( md5 STATIC MD5.cpp )
TARGET_COMPILE_OPTIONS( md5 PRIVATE -w )
TARGET_INCLUDE_DIRECTORIES( md5 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
