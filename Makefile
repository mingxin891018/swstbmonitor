#############################################
# Author: Dou Hongchen, WangChao            #
# 2006-02-23 last update by Dou Hongchen    #
# 2007-03-06 last update by WangChao	    #
#############################################
#中间间配置文件
include $(ROOTDIR_APP)/configs/$(SW_AREA)/$(SW_PLATFORM)/func_config
#模块名称
NAME := swstbmonitorserver


BUILD_TARGET_TYPE := dll
BINDIR := $(SWMODULESROOT)/pub/libs/$(SW_PLATFORM)
ifeq ($(SW_PLATFORM),android_hi3716)
BUILD_TARGET_TYPE := static
BINDIR := $(ROOTDIR)/swiptvmw_android/hwnmpd/libs/$(SW_PLATFORM)
endif

ifeq ($(SW_PLATFORM),android_hi3719)
BUILD_TARGET_TYPE := static
BINDIR := $(ROOTDIR)/swiptvmw_android/hwnmpd/libs/$(SW_PLATFORM)
endif

ifeq ($(SW_PLATFORM),android_hi3798)
BUILD_TARGET_TYPE := static
BINDIR := $(ROOTDIR)/swiptvmw_android/hwnmpd/libs/$(SW_PLATFORM)
endif

ifeq ($(SUPPORT_TM_8V8),y)  
CFLAGS += -DSUPPORT_TM_8V8
endif

ifeq ($(SUPPORT_OTT),y)
CFLAGS += -DSUPPORT_OTT
endif

ifeq ($(SUPPORT_C60),y)
CFLAGS += -DSUPPORT_C60
endif

SRC += swstbmonitor_aes.c  
SRC += swstbmonitor_md5.c  
SRC += swstbmonitor_server.c  
SRC += swstbmonitor_session.c  
SRC += swstbmonitor_upgrade_server.c
ifeq ($(SW_PLATFORM),android_hi3716)
SRC += swstbmonitor_upgrade_session_android.c
else
SRC += swstbmonitor_upgrade_session.c
endif
SRC += swstbmonitor_ping.c
SRC += swstbmonitor_traceroute.c
SRC += swstbmonitor_remotepcap.c
SRC += swstbmonitor_client.c

CFLAGS +=										\
-I.			 									\
-I$(ROOTDIR_APP)/include 						\
-I$(MIDDLEWARE_DIR)/include/hwiptv 				\
-I$(SWMODULESROOT)/pub/include/swmqmc			\
-I$(SWMODULESROOT)/pub/include					\
-I$(SWMODULESROOT)/pub/include/swcwmp			\
-I$(SWAPIROOT)/pub/base/include					\
-I$(SWAPIROOT)/pub/base/include/swos			\
-I$(SWAPIROOT)/pub/common/include				\
-I$(SWAPIROOT)/pub/common/include/swutil		\
-I$(SWAPIROOT)/pub/common/include/swxml			\
-I$(SWAPIROOT)/pub/network/include				\
-I$(SWAPIROOT)/pub/extdev/include				\
-I$(SWAPIROOT)/pub/platform/include				\
-I$(SWAPIROOT)/pub/media/include				\
-I$(SWAPIROOT)/pub/media/include/swicca			\
-I$(SWAPIROOT)/pub/media/include/swicca/irdeto	\
-I$(SWAPIROOT)/pub/upgrade/include				\
-I$(SWPARTSROOT)/pub/include					\
-I$(SWPARTSROOT)/pub/include/openssl			\

ifeq ($(SUPPORT_HWSTBMONITOR_VER4),y)  
CFLAGS += -DSUPPORT_HWSTBMONITOR_VER4
endif

ifeq ($(SUPPORT_MONITOR_CHECKCODE), y)
CFLAGS += -DSUPPORT_MONITOR_CHECKCODE
endif

include $(SWAPIROOT)/build/common.Makefile
copy:
	cp -f $(SWMODULESROOT)/pub/libs/$(SW_PLATFORM)/libswstbmonitorserver.so $(RELEASEDIR)/rootfs/usr/local/lib
