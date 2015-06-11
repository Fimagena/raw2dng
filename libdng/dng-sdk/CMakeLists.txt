FIND_PACKAGE(Threads)

FIND_PACKAGE( JPEG )
INCLUDE_DIRECTORIES( ${JPEG_INCLUDE_DIR} )
ADD_DEFINITIONS(${JPEG_DEFINITIONS})

FIND_PACKAGE( ZLIB )
INCLUDE_DIRECTORIES( ${ZLIB_INCLUDE_DIR} )
ADD_DEFINITIONS(${ZLIB_DEFINITIONS})

# =======================================================
# DNG SDK source code

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/source 
                     ${CMAKE_CURRENT_SOURCE_DIR}/../xmp-sdk/public/include 
                   )

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
                    -DqLinux=1
                    -DUNIX_ENV=1

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

# Level of debug info in the console.
ADD_DEFINITIONS(-DqDNGDebug=0)
ADD_DEFINITIONS(-DqDNGValidateTarget=1)

SET( LIBDNGSDK_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_1d_function.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_1d_table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_abort_sniffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_area_task.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_bad_pixels.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_bottlenecks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_camera_profile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_color_space.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_color_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_date_time.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_exceptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_exif.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_file_stream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_filter_task.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_fingerprint.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_gain_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_globals.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_host.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_hue_sat_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_ifd.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_image_writer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_iptc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_jpeg_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_lens_correction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_linearization_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_lossless_jpeg.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_matrix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_memory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_memory_stream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_misc_opcodes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_mosaic_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_mutex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_negative.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_opcode_list.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_opcodes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_orientation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_parse_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_pixel_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_point.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_preview.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_pthread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_rational.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_read_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_rect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_ref_counted_block.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_reference.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_render.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_resample.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_shared.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_simple_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_spline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_stream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_string.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_string_list.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_tag_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_temperature.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_tile_iterator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_tone_curve.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_validate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_xmp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_xmp_sdk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_xy_coord.cpp
   )

ADD_LIBRARY( dng-sdk STATIC ${LIBDNGSDK_SRCS} )

TARGET_LINK_LIBRARIES( dng-sdk xmp-sdk ${JPEG_LIBRARIES} ${ZLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )
