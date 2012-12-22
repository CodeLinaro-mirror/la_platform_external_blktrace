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

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= 	blkprofiler.c blktrace.c act_mask.c blkparse.c blkparse_fmt.c rbtree.c \
			btt/aqd.c btt/args.c btt/bno_dump.c btt/bt_timeline.c btt/devmap.c \
			btt/devs.c btt/dip_rb.c btt/iostat.c btt/latency.c btt/misc.c btt/mmap.c \
			btt/output.c btt/plat.c btt/p_live.c btt/proc.c btt/q2d.c btt/rstats.c \
			btt/seek.c btt/trace.c btt/trace_complete.c btt/trace_im.c btt/trace_issue.c \
			btt/trace_plug.c btt/trace_queue.c btt/trace_remap.c btt/trace_requeue.c \
			btt/unplug_hist.c \

LOCAL_C_INCLUDES += 	$(LOCAL_PATH)/btt/
LOCAL_CFLAGS += -D_GNU_SOURCE -DANDROID -DPROFILER
LOCAL_MODULE := blkprofiler
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(LOCAL_PATH)/btt/Android.mk
LOCAL_PATH:= $(MY_LOCAL_PATH)
include $(LOCAL_PATH)/btreplay/Android.mk
