#ifndef __EXTRA_INFO_WIDGET_H__
#define __EXTRA_INFO_WIDGET_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "../mahjong-algorithm/fan_calculator.h"

class ExtraInfoWidget : public cocos2d::ui::Widget {
public:
    CREATE_FUNC(ExtraInfoWidget);

    int getFlowerCount() const;
    void setFlowerCount(int cnt);

    mahjong::win_flag_t getWinFlag() const;
    void setWinFlag(mahjong::win_flag_t flag);

    mahjong::wind_t getPrevalentWind() const;
    void setPrevalentWind(mahjong::wind_t wind);

    mahjong::wind_t getSeatWind() const;
    void setSeatWind(mahjong::wind_t wind);

    struct RefreshByWinTile {
        std::function<mahjong::tile_t ()> getWinTile;
        std::function<bool ()> isStandingTilesContainsServingTile;
        std::function<size_t ()> countServingTileInFixedPacks;
    };
    void refreshByKong(bool hasKong);
    void refreshByWinTile(const RefreshByWinTile &rt);

    void setParseCallback(const std::function<void (const mahjong::hand_tiles_t &, mahjong::tile_t)> &callback) {
        _parseCallback = callback;
    }

CC_CONSTRUCTOR_ACCESS:
    virtual bool init() override;

private:
    void onWinTypeGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event);
    void onFourthTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onRobKongBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onLastTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);

    void onInstructionButton(cocos2d::Ref *sender);
    void showInputAlert(const char *prevInput);
    void parseInput(const char *input);

    cocos2d::ui::RadioButtonGroup *_winTypeGroup = nullptr;
    cocos2d::ui::CheckBox *_fourthTileBox = nullptr;
    cocos2d::ui::CheckBox *_replacementBox = nullptr;
    cocos2d::ui::CheckBox *_robKongBox = nullptr;
    cocos2d::ui::CheckBox *_lastTileBox = nullptr;
    cocos2d::ui::EditBox *_editBox = nullptr;
    cocos2d::ui::RadioButtonGroup *_prevalentWindGroup = nullptr;
    cocos2d::ui::RadioButtonGroup *_seatWindGroup = nullptr;

    bool _maybeFourthTile = false;
    bool _hasKong = false;
    size_t _winTileCountInFixedPacks = 0;

    std::function<void (const mahjong::hand_tiles_t &, mahjong::tile_t)> _parseCallback;
};

#endif
