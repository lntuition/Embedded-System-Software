LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:=dev_driver
LOCAL_SRC_FILES:=dev_driver.c
LOCAL_LDLIBS := -llog
#LOCAL_LDLIB := -L$(SYSROOT)/usr/lib -llog

include $(BUILD_SHARED_LIBRARY)

