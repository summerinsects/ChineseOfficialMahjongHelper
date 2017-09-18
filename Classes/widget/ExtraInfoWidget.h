#ifndef __EXTRA_INFO_WIDGET_H__
#define __EXTRA_INFO_WIDGET_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "../mahjong-algorithm/fan_calculator.h"

class ExtraInfoWidget : public cocos2d::Node {
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
        std::function<bool ()> isFixedPacksContainsKong;
    };
    void refreshByKong(bool hasKong);
    void refreshByWinTile(const RefreshByWinTile &rt);

    virtual bool init() override;

private:
    void onWinTypeGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event);
    void onFourthTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onRobKongBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onLastTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);

    void onInstructionButton(cocos2d::Ref *sender);

    cocos2d::ui::RadioButtonGroup *_winTypeGroup = nullptr;
    cocos2d::ui::CheckBox *_fourthTileBox = nullptr;
    cocos2d::ui::CheckBox *_replacementBox = nullptr;
    cocos2d::ui::CheckBox *_robKongBox = nullptr;
    cocos2d::ui::CheckBox *_lastTileBox = nullptr;
    cocos2d::Label *_flowerLabel = nullptr;
    cocos2d::ui::RadioButtonGroup *_windGroups[2];

    bool _maybeFourthTile = false;
    bool _hasKong = false;
    size_t _winTileCountInFixedPacks = 0;
};

#endif
