#模块名称
NAME := swstbmonitor
BUILD_TARGET_TYPE := exe
BINDIR := $(SWAPIROOT)/pub/libs/$(SW_PLATFORM)

SRC = hwnmpd.c

INCDIR +=										\
-I.			 									\
-I$(ROOTDIR_APP)/include 						\
-I$(SWMODULESROOT)/pub/include					\
-I$(SWMODULESROOT)/pub/swstbmonitor/include		\
-I$(SWAPIROOT)/pub/base/include					\
-I$(SWAPIROOT)/pub/base/include/swos			\
-I$(SWAPIROOT)/pub/common/include				\
-I$(SWAPIROOT)/pub/common/include/swutil		\
-I$(SWAPIROOT)/pub/common/include/swxml			\
-I$(SWAPIROOT)/pub/network/include				\
-I$(SWAPIROOT)/pub/extdev/include				\
-I$(SWAPIROOT)/pub/platform/include				\
-I$(SWAPIROOT)/pub/upgrade/include				\
-I$(SWPARTSROOT)/pub/include					\

DYN_LDS_WITH += -L$(SWMODULESROOT)/pub/libs/$(SW_PLATFORM) -lswsysclient \
				-L$(SWMODULESROOT)/pub/libs/$(SW_PLATFORM) -lswmodules \
				-L$(SWAPIROOT)/pub/base/libs/$(SW_PLATFORM) -lswbase -lswos \
				-L$(SWAPIROOT)/pub/common/libs/$(SW_PLATFORM) -lswcommon -lswparameter -lswutil\
				-L$(SWAPIROOT)/pub/media/libs/$(SW_PLATFORM)/ -lswmuxer \
				-L$(SWAPIROOT)/pub/media/libs/$(SW_PLATFORM)/ffmpeg-$(FFMPEG_VERSION) -lavutil -lavformat -lavcodec \
				-L$(SWAPIROOT)/pub/platform/libs/$(SW_PLATFORM)/ -lswplatform \
				-L$(SWAPIROOT)/pub/swdevinfo/libs/$(SW_PLATFORM)/ -lswdevinfo \
				-L$(SWAPIROOT)/pub/network/libs/$(SW_PLATFORM)/ -lswwifimanager \
				-L$(SWMODULESROOT)/pub/libs/$(SW_PLATFORM) -lswqrcodec \
				-L$(SWPARTSROOT)/pub/libs/$(SW_PLATFORM)/iconv-1.16 -liconv \
				-L$(SWPARTSROOT)/pub/libs/$(SW_PLATFORM)/zbar-0.10 -lzbar \
				-L$(SWPARTSROOT)/pub/libs/$(SW_PLATFORM)/freetype-2.8.1 -lfreetype \
				-L$(SWAPIROOT)/pub/network/libs/$(SW_PLATFORM) -ldhcpc -lswnetwork -lswnetutil

DYN_LDS_WITH+= -L$(SWAPIROOT)/pub/platform/libs/$(SW_PLATFORM)/sdk \
			   -lrtstream \
			   -lrtsvideo \
			   -lrtsaudio \
			   -lrtsaec \
			   -lrtsaec_chn \
			   -laacenc \
			   -lasound \
			   -lrtsio


ifeq ($(SUPPORT_OPENSSL),y)
INCDIR += -I$(SWPARTSROOT)/pub/include/$(OPENSSL_VER)/
DYN_LDS_WITH += -L$(SWPARTSROOT)/pub/libs/$(SW_PLATFORM)/$(OPENSSL_VER)  -lcrypto -lssl
CFLAGS += -DSUPPORT_OPENSSL
endif

ifeq ($(SUPPORT_WOLFSSL),y)
CFLAGS += -DSUPPORT_WOLFSSL
INCDIR += -I$(SWPARTSROOT)/pub/include/$(WOLFSSL_VER)/
DYN_LDS_WITH += -L$(SWPARTSROOT)/pub/libs/$(SW_PLATFORM)/$(WOLFSSL_VER) -lwolfssl
endif

include $(SWAPIROOT)/build/common.Makefile
