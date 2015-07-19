LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := raw2dng
LOCAL_STATIC_LIBRARIES := dng
LOCAL_SHARED_LIBRARIES := exiv2 raw
LOCAL_LDLIBS += -lz -llog
LOCAL_CPP_FEATURES += exceptions
LOCAL_SRC_FILES := raw2dng.cpp \
                   negativeProcessor.cpp \
                   vendorProcessors/ILCE7processor.cpp \
                   vendorProcessors/FujiProcessor.cpp \
                   vendorProcessors/variousVendorProcessor.cpp
include $(BUILD_SHARED_LIBRARY)

$(call import-module,raw2dng/libdng)
$(call import-module,exiv2-0.25)
$(call import-module,LibRaw-0.17.0-Beta2)

