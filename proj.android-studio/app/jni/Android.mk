LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d)
$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d/external)
$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d/cocos)
$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d/cocos/audio/include)

LOCAL_MODULE := MyGame_shared

LOCAL_MODULE_FILENAME := libMyGame

LOCAL_SRC_FILES := $(LOCAL_PATH)/hellocpp/main.cpp \
                   $(LOCAL_PATH)/../../../Classes/AppDelegate.cpp \
                   $(LOCAL_PATH)/../../../Classes/cocos-wheels/CWCommon.cpp \
                   $(LOCAL_PATH)/../../../Classes/cocos-wheels/CWTableView.cpp \
                   $(LOCAL_PATH)/../../../Classes/CompetitionSystem/CompetitionMainScene.cpp \
                   $(LOCAL_PATH)/../../../Classes/CompetitionSystem/LatestCompetitionScene.cpp \
                   $(LOCAL_PATH)/../../../Classes/FanCalculator/FanCalculatorScene.cpp \
                   $(LOCAL_PATH)/../../../Classes/FanTable/FanDefinitionScene.cpp \
                   $(LOCAL_PATH)/../../../Classes/FanTable/FanTableScene.cpp \
                   $(LOCAL_PATH)/../../../Classes/HelloWorldScene.cpp \
                   $(LOCAL_PATH)/../../../Classes/mahjong-algorithm/fan_calculator.cpp \
                   $(LOCAL_PATH)/../../../Classes/mahjong-algorithm/stringify.cpp \
                   $(LOCAL_PATH)/../../../Classes/mahjong-algorithm/shanten.cpp \
                   $(LOCAL_PATH)/../../../Classes/MahjongTheory/MahjongTheoryScene.cpp \
                   $(LOCAL_PATH)/../../../Classes/Other/OtherScene.cpp \
                   $(LOCAL_PATH)/../../../Classes/RecordSystem/Record.cpp \
                   $(LOCAL_PATH)/../../../Classes/RecordSystem/RecordHistoryScene.cpp \
                   $(LOCAL_PATH)/../../../Classes/RecordSystem/RecordScene.cpp \
                   $(LOCAL_PATH)/../../../Classes/RecordSystem/ScoreSheetScene.cpp \
                   $(LOCAL_PATH)/../../../Classes/utils/common.cpp \
                   $(LOCAL_PATH)/../../../Classes/widget/AlertView.cpp \
                   $(LOCAL_PATH)/../../../Classes/widget/ExtraInfoWidget.cpp \
                   $(LOCAL_PATH)/../../../Classes/widget/HandTilesWidget.cpp \
                   $(LOCAL_PATH)/../../../Classes/widget/TilePickWidget.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../Classes

# _COCOS_HEADER_ANDROID_BEGIN
# _COCOS_HEADER_ANDROID_END


LOCAL_STATIC_LIBRARIES := cocos2dx_static

# _COCOS_LIB_ANDROID_BEGIN
# _COCOS_LIB_ANDROID_END

include $(BUILD_SHARED_LIBRARY)

$(call import-module,.)

# _COCOS_LIB_IMPORT_ANDROID_BEGIN
# _COCOS_LIB_IMPORT_ANDROID_END
