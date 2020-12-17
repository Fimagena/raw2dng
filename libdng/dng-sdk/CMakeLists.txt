FIND_PACKAGE(Threads)

FIND_PACKAGE( JPEG )
INCLUDE_DIRECTORIES( ${JPEG_INCLUDE_DIR} )
ADD_DEFINITIONS( ${JPEG_DEFINITIONS} )

FIND_PACKAGE( ZLIB )
INCLUDE_DIRECTORIES( ${ZLIB_INCLUDE_DIR} )
ADD_DEFINITIONS( ${ZLIB_DEFINITIONS} )

# =======================================================
# DNG SDK source code

ADD_LIBRARY( dng-sdk STATIC
             ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_1d_function.cpp
             ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_1d_table.cpp
             ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_abort_sniffer.cpp
             ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_area_task.cpp
             ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_bad_pixels.cpp
             ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_big_table.cpp
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
             ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_local_string.cpp
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
             ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_safe_arithmetic.cpp
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
             ${CMAKE_CURRENT_SOURCE_DIR}/source/dng_xy_coord.cpp )

TARGET_INCLUDE_DIRECTORIES( dng-sdk INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/source )

TARGET_COMPILE_DEFINITIONS( dng-sdk PUBLIC
                            -DqWinOS=0 -DqMacOS=0 -DqLinux=1   # platform for dng-sdk
                            -DqDNGThreadSafe=1                 # we're thread-safe - will use pthreads
#                           -DqDNG64Bit=1                      # set manually to support 64bit platforms (todo: add test)
                            -DqDNGDebug=0                      # do not compile debug code
                            -DqDNGValidateTarget=0             # do not build dng_validate-binary
                            -DqDNGUseLibJPEG=1                 # use libjpeg
                            -DqDNGXMPFiles=0 -DqDNGXMPDocOps=0 # build minimal XMP-set
                            -DqDNGUseStdInt=1 )                # Must be set to 1, else do not compile under Linux.
# Check processor endianness (todo: not sure bigEndian platforms are set correctly)
INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
IF(NOT IS_BIG_ENDIAN)
    ADD_DEFINITIONS(-DqDNGLittleEndian=1)
ENDIF(NOT IS_BIG_ENDIAN)

TARGET_COMPILE_OPTIONS( dng-sdk PRIVATE -fexceptions )

TARGET_LINK_LIBRARIES( dng-sdk PRIVATE xmp-sdk ${JPEG_LIBRARIES} ${ZLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} )
