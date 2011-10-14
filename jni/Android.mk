LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
OPENCV_CAMERA_MODULES:=off
include /home/kintaro/android/OpenCV-2.3.1/share/OpenCV/OpenCV.mk


LOCAL_MODULE    := descriptor
LOCAL_SRC_FILES := punto.cpp lista_puntos.cpp descriptor.cpp
LOCAL_LDLIBS    := -llog -ldl 
include $(BUILD_SHARED_LIBRARY)