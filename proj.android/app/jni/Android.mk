LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d)
$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d/external)
$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d/cocos)
$(call import-add-path,$(LOCAL_PATH)/../../../cocos2d/cocos/audio/include)

LOCAL_MODULE := MyGame_shared

LOCAL_MODULE_FILENAME := libMyGame

LOCAL_SRC_FILES := hellocpp/main.cpp \
                   ../../../Classes/AppDelegate.cpp \
                   ../../../Classes/cocos-wheels/CWCommon-android.cpp \
                   ../../../Classes/cocos-wheels/CWCommon.cpp \
                   ../../../Classes/cocos-wheels/CWTableView.cpp \
                   ../../../Classes/CompetitionSystem/CompetitionMainScene.cpp \
                   ../../../Classes/CompetitionSystem/LatestCompetitionScene.cpp \
                   ../../../Classes/FanCalculator/FanCalculatorScene.cpp \
                   ../../../Classes/FanTable/FanTableScene.cpp \
                   ../../../Classes/HelloWorldScene.cpp \
                   ../../../Classes/mahjong-algorithm/fan_calculator.cpp \
                   ../../../Classes/mahjong-algorithm/stringify.cpp \
                   ../../../Classes/mahjong-algorithm/shanten.cpp \
                   ../../../Classes/MahjongTheory/MahjongTheoryScene.cpp \
                   ../../../Classes/Other/OtherScene.cpp \
                   ../../../Classes/RecordSystem/Record.cpp \
                   ../../../Classes/RecordSystem/RecordHistoryScene.cpp \
                   ../../../Classes/RecordSystem/RecordScene.cpp \
                   ../../../Classes/RecordSystem/ScoreSheetScene.cpp \
                   ../../../Classes/utils/common.cpp \
                   ../../../Classes/widget/AlertDialog.cpp \
                   ../../../Classes/widget/CommonWebViewScene.cpp \
                   ../../../Classes/widget/ExtraInfoWidget.cpp \
                   ../../../Classes/widget/HandTilesWidget.cpp \
                   ../../../Classes/widget/PopupMenu.cpp \
                   ../../../Classes/widget/TilePickWidget.cpp \
                   ../../../Classes/widget/Toast.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../Classes

# _COCOS_HEADER_ANDROID_BEGIN
# _COCOS_HEADER_ANDROID_END


LOCAL_STATIC_LIBRARIES := cocos2dx_static

# _COCOS_LIB_ANDROID_BEGIN
# _COCOS_LIB_ANDROID_END

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path, $(LOCAL_PATH)/../../../cocos2d)
$(call import-module, cocos)

# _COCOS_LIB_IMPORT_ANDROID_BEGIN
# _COCOS_LIB_IMPORT_ANDROID_END
