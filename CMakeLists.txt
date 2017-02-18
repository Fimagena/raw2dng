PROJECT( raw2dng )

CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake-modules")

ADD_COMPILE_OPTIONS( -Wall )

ADD_SUBDIRECTORY( libdng )
ADD_SUBDIRECTORY( raw2dng )
