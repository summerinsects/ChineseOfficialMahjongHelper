#ifndef _POINTS_CALCULATOR_SCENE_H_
#define _POINTS_CALCULATOR_SCENE_H_

#include "../BaseLayer.h"

class TilePickWidget;

class PointsCalculatorScene : public BaseLayer {
public:
    static cocos2d::Scene *createScene();

    virtual bool init() override;

    CREATE_FUNC(PointsCalculatorScene);

private:
    void onWinTypeGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event);
    void onFourthTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onRobKongBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onReplacementBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onLastTileBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);

    void onFixedSetsChanged();
    void onWinTileChanged();

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

    cocos2d::Node *_pointsAreaNode = nullptr;

    bool _maybeFourthTile = false;
    bool _hasKong = false;
    bool _maybeRobKong = false;
};

#endif
