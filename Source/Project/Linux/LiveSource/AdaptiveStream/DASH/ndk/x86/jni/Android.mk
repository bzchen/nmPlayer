LOCAL_PATH := $(call my-dir)
VOTOP?=../../../../../../../../..

include $(CLEAR_VARS)

LOCAL_MODULE := voAdaptiveStreamDASH

CMNSRC_PATH:=../../../../../../../../../Common
CSRC_PATH:=../../../../../../../../../Source

LOCAL_SRC_FILES := \
				   $(CMNSRC_PATH)/CvoBaseObject.cpp \
				   $(CMNSRC_PATH)/voLog.c \
				   $(CMNSRC_PATH)/voCMutex.cpp \
				   $(CMNSRC_PATH)/voOSFunc.cpp \
				   $(CMNSRC_PATH)/voXMLLoad.cpp \
				   $(CMNSRC_PATH)/XMLSaxDefaultHandler.cpp \
				   $(CMNSRC_PATH)/CDllLoad.cpp \
				   $(CMNSRC_PATH)/voHalInfo.cpp \
				   $(CSRC_PATH)/Common/vo_thread.cpp \
				   $(CSRC_PATH)/Common/CDataBox.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/VO_MPD_Parser.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/C_MPD_Manager.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/C_DASH_Entity.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/voAdaptiveStreamDASH.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/Mpd_tag.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/BaseUrl.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/Common_Tag.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/contentComponent_Tag.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/Group_Tag.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/Metrics.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/Period_Tag.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/ProgramInformation.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/Repre_Tag.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/Role_Tag.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/SegInfo_Tag.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/SegList_Tag.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/SegmentBase_Tag.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/SegmentTemplate_Tag.cpp \
				   $(CSRC_PATH)/AdaptiveStreaming/Dash/SubRepre_Tag.cpp \



LOCAL_C_INCLUDES := \
					../../../../../../../../../Include \
					../../../../../../../../../Include/vome \
					../../../../../../../../../Common \
					../../../../../../../../../Thirdparty/ndk \
					../../../../../../../../../Include/vome \
					../../../../../../../../../Common \
					../../../../../../../../../Common/NetWork \
					../../../../../../../../../MFW/voME/Common \
					$(CSRC_PATH)/AdaptiveStreaming/Include \
					$(CSRC_PATH)/Common \
					$(CSRC_PATH)/Include \
					$(CSRC_PATH)/AdaptiveStreaming/Dash \
					$(CSRC_PATH)/AdaptiveStreaming/Common \
					$(CSRC_PATH)/File/Common \
					$(CSRC_PATH)/File/Common/Utility \
					$(CSRC_PATH)/File/XML \


        
LOCAL_STATIC_LIBRARIES := cpufeatures


VOMM:= -DLINUX -D_LINUX -DHAVE_PTHREADS -D_LINUX_ANDROID -D_VOLOG_ERROR -D_VOLOG_WARNING -D_VOLOG_INFO -D_VOLOG_RUN -D_DASH_SOURCE_ -D__VO_NDK__  -D_new_programinfo

# about info option, do not need to care it
LOCAL_CFLAGS := -D_VOMODULEID=0x01123000  -DNDEBUG -march=i686 -mtune=atom -mstackrealign -msse3 -mfpmath=sse -m32
LOCAL_LDLIBS := ../../../../../../../../../Lib/ndk/x86/libvoCheck.a -llog -ldl -lstdc++ -lgcc -L../../../../../../../../../Lib/ndk/x86/ -lvodl

include $(VOTOP)/build/vondk.mk
include $(BUILD_SHARED_LIBRARY)

$(call import-module,cpufeatures)
