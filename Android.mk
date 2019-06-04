##############################################################################
#
#    Copyright (c) 2005 - 2019 by Vivante Corp.  All rights reserved.
#
#    The material in this file is confidential and contains trade secrets
#    of Vivante Corporation. This is proprietary information owned by
#    Vivante Corporation. No part of this work may be disclosed,
#    reproduced, copied, transmitted, or used in any way for any purpose,
#    without the express written permission of Vivante Corporation.
#
##############################################################################
LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_PRODUCT), galilei)
RRODUCT_PATH := libraryso/PID0x88
else
RRODUCT_PATH := libraryso/PID0x99
endif


include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    libraryso/libCLC.so
LOCAL_MODULE         := libCLC
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    libraryso/libGAL.so
LOCAL_MODULE         := libGAL
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    libraryso/libLLVM_viv.so
LOCAL_MODULE         := libLLVM_viv
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    $(RRODUCT_PATH)/libNNVXCBinary.so
LOCAL_MODULE         := libNNVXCBinary
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    libraryso/libOpenCL.so
LOCAL_MODULE         := libOpenCL
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    $(RRODUCT_PATH)/libOpenVX.so
LOCAL_MODULE         := libOpenVX
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    libraryso/libOpenVXU.so
LOCAL_MODULE         := libOpenVXU
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    libraryso/libovxlib.so
LOCAL_MODULE         := libovxlib
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    libraryso/libVSC.so
LOCAL_MODULE         := libVSC
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    $(RRODUCT_PATH)/libOvx12VXCBinary.so
LOCAL_MODULE         := libOvx12VXCBinary
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib
else
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif
include $(BUILD_PREBUILT)
ifeq ($(BOARD_NPU_SERVICE_ENABLE), true)
include $(LOCAL_PATH)/service/Android.mk
endif
