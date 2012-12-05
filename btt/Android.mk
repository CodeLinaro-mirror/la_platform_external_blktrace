LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= 	aqd.c \
			args.c \
			bno_dump.c \
			bt_timeline.c \
			devmap.c \
			devs.c \
			dip_rb.c \
			iostat.c \
			latency.c \
			misc.c \
			mmap.c \
			output.c \
			plat.c \
			p_live.c \
			proc.c \
			q2d.c \
			rstats.c \
			seek.c \
			trace.c \
			trace_complete.c \
			trace_im.c \
			trace_issue.c \
			trace_plug.c \
			trace_queue.c \
			trace_remap.c \
			trace_requeue.c \
			unplug_hist.c \
			../rbtree.c \

LOCAL_C_INCLUDES += 	$(LOCAL_PATH)/..
LOCAL_CFLAGS += -D_GNU_SOURCE -DANDROID
LOCAL_MODULE := btt
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
