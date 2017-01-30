LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

$(call import-add-path,$(LOCAL_PATH)/../../cocos2d)
$(call import-add-path,$(LOCAL_PATH)/../../cocos2d/external)
$(call import-add-path,$(LOCAL_PATH)/../../cocos2d/cocos)

LOCAL_MODULE := cocos2dcpp_shared

LOCAL_MODULE_FILENAME := libcocos2dcpp

LOCAL_SRC_FILES := hellocpp/main.cpp \
                   ../../Classes/mahjong-algorithm/points_calculator.cpp \
                   ../../Classes/mahjong-algorithm/wait_and_win.cpp \
                   ../../Classes/Other/OtherScene.cpp \
                   ../../Classes/PointsCalculator/PointsCalculatorScene.cpp \
                   ../../Classes/ScoreSheet/HistoryScene.cpp \
                   ../../Classes/ScoreSheet/RecordScene.cpp \
                   ../../Classes/ScoreSheet/ScoreSheetScene.cpp \
                   ../../Classes/ScoreTable/ScoreDefinition.cpp \
                   ../../Classes/ScoreTable/ScoreTable.cpp \
                   ../../Classes/widget/AlertLayer.cpp \
                   ../../Classes/widget/CWTableView.cpp \
                   ../../Classes/widget/HandTilesWidget.cpp \
                   ../../Classes/widget/TilePickWidget.cpp \
                   ../../Classes/AppDelegate.cpp \
                   ../../Classes/HelloWorldScene.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../Classes

# _COCOS_HEADER_ANDROID_BEGIN
# _COCOS_HEADER_ANDROID_END


LOCAL_STATIC_LIBRARIES := cocos2dx_static

# _COCOS_LIB_ANDROID_BEGIN
# _COCOS_LIB_ANDROID_END

include $(BUILD_SHARED_LIBRARY)

$(call import-module,.)

# _COCOS_LIB_IMPORT_ANDROID_BEGIN
# _COCOS_LIB_IMPORT_ANDROID_END
