LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE    := descriptor
LOCAL_SRC_FILES := punto.c lista_puntos.c ext_cv_funcs.c descriptor.c
LOCAL_LDLIBS    := -llog -ljnigraphics
include $(BUILD_SHARED_LIBRARY)