LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

$(call import-add-path,$(LOCAL_PATH)/../../cocos2d)
$(call import-add-path,$(LOCAL_PATH)/../../cocos2d/external)
$(call import-add-path,$(LOCAL_PATH)/../../cocos2d/cocos)
$(call import-add-path,$(LOCAL_PATH)/../../cocos2d/cocos/audio/include)

LOCAL_MODULE := MyGame_shared

LOCAL_MODULE_FILENAME := libMyGame

LOCAL_SRC_FILES := hellocpp/main.cpp \
                   ../../Classes/CompetitionSystem/Competition.cpp \
                   ../../Classes/CompetitionSystem/CompetitionEnrollScene.cpp \
                   ../../Classes/CompetitionSystem/CompetitionMainScene.cpp \
                   ../../Classes/CompetitionSystem/CompetitionRankCustomScene.cpp \
                   ../../Classes/CompetitionSystem/CompetitionRoundScene.cpp \
                   ../../Classes/CompetitionSystem/CompetitionTableScene.cpp \
                   ../../Classes/FanCalculator/FanCalculatorScene.cpp \
                   ../../Classes/FanTable/FanDefinitionScene.cpp \
                   ../../Classes/FanTable/FanTableScene.cpp \
                   ../../Classes/LatestCompetition/LatestCompetitionScene.cpp \
                   ../../Classes/mahjong-algorithm/fan_calculator.cpp \
                   ../../Classes/mahjong-algorithm/stringify.cpp \
                   ../../Classes/mahjong-algorithm/shanten.cpp \
                   ../../Classes/MahjongTheory/MahjongTheoryScene.cpp \
                   ../../Classes/Other/OtherScene.cpp \
                   ../../Classes/ScoreSheet/HistoryScene.cpp \
                   ../../Classes/ScoreSheet/Record.cpp \
                   ../../Classes/ScoreSheet/RecordScene.cpp \
                   ../../Classes/ScoreSheet/ScoreSheetScene.cpp \
                   ../../Classes/widget/AlertView.cpp \
                   ../../Classes/widget/CWTableView.cpp \
                   ../../Classes/widget/ExtraInfoWidget.cpp \
                   ../../Classes/widget/HandTilesWidget.cpp \
                   ../../Classes/widget/TilePickWidget.cpp \
                   ../../Classes/widget/TilesKeyboard.cpp \
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
