#ifndef __FAN_CALCULATOR_SCENE_H__
#define __FAN_CALCULATOR_SCENE_H__

#include "../BaseLayer.h"

class TilePickWidget;

class FanCalculatorScene : public BaseLayer {
public:
    static cocos2d::Scene *createScene();

    virtual bool init() override;

    CREATE_FUNC(FanCalculatorScene);

private:
    void onWinTypeGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event);
    void onFourthTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onRobKongBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onLastTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);

    void onFixedPacksChanged();
    void onWinTileChanged();

    void onInstructionButton(cocos2d::Ref *sender);
    void showInputAlert(const char *prevInput);
    void parseInput(const char *input);
    void calculate();

    TilePickWidget *_tilePicker = nullptr;
    cocos2d::ui::RadioButtonGroup *_winTypeGroup = nullptr;
    cocos2d::ui::CheckBox *_fourthTileBox = nullptr;
    cocos2d::ui::CheckBox *_replacementBox = nullptr;
    cocos2d::ui::CheckBox *_robKongBox = nullptr;
    cocos2d::ui::CheckBox *_lastTileBox = nullptr;
    cocos2d::ui::EditBox *_editBox = nullptr;
    cocos2d::ui::RadioButtonGroup *_prevalentWindGroup = nullptr;
    cocos2d::ui::RadioButtonGroup *_seatWindGroup = nullptr;

    cocos2d::Node *_fanAreaNode = nullptr;

    bool _maybeFourthTile = false;
    bool _hasKong = false;
    size_t _winTileCountInFixedPacks = 0;
};

#endif
