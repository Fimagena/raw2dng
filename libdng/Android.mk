LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := md5
LOCAL_SRC_FILES := md5/MD5.cpp
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/md5
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := xmp-sdk
LOCAL_STATIC_LIBRARIES := md5 expat
LOCAL_CFLAGS := -DUNIX_ENV=1 -DkBigEndianHost=0
LOCAL_EXPORT_CFLAGS := -DUNIX_ENV=1
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/xmp-sdk/public/include
LOCAL_CPP_FEATURES += exceptions
LOCAL_C_INCLUDES := $(LOCAL_PATH)/xmp-sdk \
                    $(LOCAL_PATH)/xmp-sdk/public/include 
LOCAL_SRC_FILES := xmp-sdk/source/Host_IO-POSIX.cpp \
                   xmp-sdk/source/IOUtils.cpp \
                   xmp-sdk/source/PerfUtils.cpp \
                   xmp-sdk/source/SafeStringAPIs.cpp \
                   xmp-sdk/source/UnicodeConversions.cpp \
                   xmp-sdk/source/XIO.cpp \
                   xmp-sdk/source/XML_Node.cpp \
                   xmp-sdk/source/XMP_LibUtils.cpp \
                   xmp-sdk/source/XMP_ProgressTracker.cpp \
                   xmp-sdk/source/XMPFiles_IO.cpp \
                   xmp-sdk/XMPCore/source/ExpatAdapter.cpp \
                   xmp-sdk/XMPCore/source/ParseRDF.cpp \
                   xmp-sdk/XMPCore/source/WXMPIterator.cpp \
                   xmp-sdk/XMPCore/source/WXMPMeta.cpp \
                   xmp-sdk/XMPCore/source/WXMPUtils.cpp \
                   xmp-sdk/XMPCore/source/XMPCore_Impl.cpp \
                   xmp-sdk/XMPCore/source/XMPIterator.cpp \
                   xmp-sdk/XMPCore/source/XMPMeta-GetSet.cpp \
                   xmp-sdk/XMPCore/source/XMPMeta-Parse.cpp \
                   xmp-sdk/XMPCore/source/XMPMeta-Serialize.cpp \
                   xmp-sdk/XMPCore/source/XMPMeta.cpp \
                   xmp-sdk/XMPCore/source/XMPUtils-FileInfo.cpp \
                   xmp-sdk/XMPCore/source/XMPUtils.cpp
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := dng-sdk
LOCAL_STATIC_LIBRARIES := xmp-sdk jpeg
LOCAL_CFLAGS := -DqWinOS=0 -DqAndroid=1 -DqMacOS=0 -DqDNGLittleEndian=1 -DqDNGThreadSafe=1 -DqDNGXMPDocOps=0
LOCAL_EXPORT_CFLAGS := -DqWinOS=0 -DqAndroid=1 -DqMacOS=0 -DqDNGLittleEndian=1 -DqDNGThreadSafe=1 -DqDNGXMPDocOps=0
LOCAL_CPP_FEATURES += exceptions
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/dng-sdk/source
#LOCAL_LDLIBS += -lz
LOCAL_SRC_FILES := dng-sdk/source/dng_1d_function.cpp \
                   dng-sdk/source/dng_1d_table.cpp \
                   dng-sdk/source/dng_abort_sniffer.cpp \
                   dng-sdk/source/dng_area_task.cpp \
                   dng-sdk/source/dng_bad_pixels.cpp \
                   dng-sdk/source/dng_bottlenecks.cpp \
                   dng-sdk/source/dng_camera_profile.cpp \
                   dng-sdk/source/dng_color_space.cpp \
                   dng-sdk/source/dng_color_spec.cpp \
                   dng-sdk/source/dng_date_time.cpp \
                   dng-sdk/source/dng_exceptions.cpp \
                   dng-sdk/source/dng_exif.cpp \
                   dng-sdk/source/dng_file_stream.cpp \
                   dng-sdk/source/dng_filter_task.cpp \
                   dng-sdk/source/dng_fingerprint.cpp \
                   dng-sdk/source/dng_gain_map.cpp \
                   dng-sdk/source/dng_globals.cpp \
                   dng-sdk/source/dng_host.cpp \
                   dng-sdk/source/dng_hue_sat_map.cpp \
                   dng-sdk/source/dng_ifd.cpp \
                   dng-sdk/source/dng_image.cpp \
                   dng-sdk/source/dng_image_writer.cpp \
                   dng-sdk/source/dng_info.cpp \
                   dng-sdk/source/dng_iptc.cpp \
                   dng-sdk/source/dng_jpeg_image.cpp \
                   dng-sdk/source/dng_lens_correction.cpp \
                   dng-sdk/source/dng_linearization_info.cpp \
                   dng-sdk/source/dng_lossless_jpeg.cpp \
                   dng-sdk/source/dng_matrix.cpp \
                   dng-sdk/source/dng_memory.cpp \
                   dng-sdk/source/dng_memory_stream.cpp \
                   dng-sdk/source/dng_misc_opcodes.cpp \
                   dng-sdk/source/dng_mosaic_info.cpp \
                   dng-sdk/source/dng_mutex.cpp \
                   dng-sdk/source/dng_negative.cpp \
                   dng-sdk/source/dng_opcode_list.cpp \
                   dng-sdk/source/dng_opcodes.cpp \
                   dng-sdk/source/dng_orientation.cpp \
                   dng-sdk/source/dng_parse_utils.cpp \
                   dng-sdk/source/dng_pixel_buffer.cpp \
                   dng-sdk/source/dng_point.cpp \
                   dng-sdk/source/dng_preview.cpp \
                   dng-sdk/source/dng_pthread.cpp \
                   dng-sdk/source/dng_rational.cpp \
                   dng-sdk/source/dng_read_image.cpp \
                   dng-sdk/source/dng_rect.cpp \
                   dng-sdk/source/dng_ref_counted_block.cpp \
                   dng-sdk/source/dng_reference.cpp \
                   dng-sdk/source/dng_render.cpp \
                   dng-sdk/source/dng_resample.cpp \
                   dng-sdk/source/dng_shared.cpp \
                   dng-sdk/source/dng_simple_image.cpp \
                   dng-sdk/source/dng_spline.cpp \
                   dng-sdk/source/dng_stream.cpp \
                   dng-sdk/source/dng_string.cpp \
                   dng-sdk/source/dng_string_list.cpp \
                   dng-sdk/source/dng_tag_types.cpp \
                   dng-sdk/source/dng_temperature.cpp \
                   dng-sdk/source/dng_tile_iterator.cpp \
                   dng-sdk/source/dng_tone_curve.cpp \
                   dng-sdk/source/dng_utils.cpp \
                   dng-sdk/source/dng_validate.cpp \
                   dng-sdk/source/dng_xmp.cpp \
                   dng-sdk/source/dng_xmp_sdk.cpp \
                   dng-sdk/source/dng_xy_coord.cpp
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := dng
LOCAL_STATIC_LIBRARIES := dng-sdk
#LOCAL_LDLIBS += -lz # only if building as shared library
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_SRC_FILES := dnghost.cpp
LOCAL_CPP_FEATURES += exceptions
include $(BUILD_STATIC_LIBRARY)

$(call import-module,platform_external_expat)
$(call import-module,jpeg-9a)

