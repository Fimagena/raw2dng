FIND_PACKAGE( EXPAT )
INCLUDE_DIRECTORIES( ${EXPAT_INCLUDE_DIR} )
ADD_DEFINITIONS(${EXPAT_DEFINITIONS})

# =======================================================
# XMP SDK source code.

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/public/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/../md5
                    ${CMAKE_CURRENT_SOURCE_DIR}
                   )

# Set platforms env.
IF(WIN32)
    ADD_DEFINITIONS(
                    # Set Windows rules.
                    -DWIN_ENV=1
                   )
ELSE(WIN32)
    ADD_DEFINITIONS(
                    # Set MACOS-X, Linux & co rules.
                    -DUNIX_ENV=1
                   )
ENDIF(WIN32)

SET( LIBXMP-SDK_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/source/Host_IO-POSIX.cpp
#   ${CMAKE_CURRENT_SOURCE_DIR}/source/Host_IO-Win.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/IOUtils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/PerfUtils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/SafeStringAPIs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/UnicodeConversions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/XIO.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/XML_Node.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/XMP_LibUtils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/XMP_ProgressTracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/XMPFiles_IO.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMPCore/source/ExpatAdapter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMPCore/source/ParseRDF.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMPCore/source/WXMPIterator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMPCore/source/WXMPMeta.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMPCore/source/WXMPUtils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMPCore/source/XMPCore_Impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMPCore/source/XMPIterator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMPCore/source/XMPMeta-GetSet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMPCore/source/XMPMeta-Parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMPCore/source/XMPMeta-Serialize.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMPCore/source/XMPMeta.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMPCore/source/XMPUtils-FileInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMPCore/source/XMPUtils.cpp
   )

ADD_LIBRARY( xmp-sdk STATIC ${LIBXMP-SDK_SRCS} )

TARGET_LINK_LIBRARIES( xmp-sdk md5 ${EXPAT_LIBRARIES} )
