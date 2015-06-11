PROJECT( raw2dng )

CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake-modules")

IF( MINGW OR UNIX )
    ADD_DEFINITIONS(-Wall
                    -Wcast-align
                    -Wpointer-arith
                    -Wformat-security
                    -Wmissing-format-attribute
                    -Woverloaded-virtual
                    -W
                   )
ENDIF( MINGW OR UNIX )

ADD_SUBDIRECTORY( libdng )
ADD_SUBDIRECTORY( raw2dng )
