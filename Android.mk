MY_LOCAL_PATH:= $(call my-dir)
LOCAL_PATH:= $(MY_LOCAL_PATH)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= blktrace.c act_mask.c
LOCAL_CFLAGS += -D_GNU_SOURCE -DANDROID
LOCAL_MODULE := blktrace
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= blkparse.c blkparse_fmt.c rbtree.c act_mask.c
LOCAL_CFLAGS += -D_GNU_SOURCE -DANDROID
LOCAL_MODULE := blkparse
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= verify_blkparse.c
LOCAL_CFLAGS += -D_GNU_SOURCE -DANDROID
LOCAL_MODULE := verify_blkparse
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= blkrawverify.c
LOCAL_CFLAGS += -D_GNU_SOURCE -DANDROID
LOCAL_MODULE := blkrawverify
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(LOCAL_PATH)/btt/Android.mk
LOCAL_PATH:= $(MY_LOCAL_PATH)
include $(LOCAL_PATH)/btreplay/Android.mk
