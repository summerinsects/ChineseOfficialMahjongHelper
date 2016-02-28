#ifndef _POINTS_CALCULATOR_SCENE_H_
#define _POINTS_CALCULATOR_SCENE_H_

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class PointsCalculatorScene : public cocos2d::Layer {
public:
    static cocos2d::Scene *createScene();

    virtual bool init() override;

    virtual void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* unused_event) override;

    CREATE_FUNC(PointsCalculatorScene);

private:
    void calculate();

    cocos2d::ui::EditBox *_editBox = nullptr;
    cocos2d::ui::CheckBox *_prevalentBox[4];
    cocos2d::ui::CheckBox *_seatBox[4];
    cocos2d::ui::CheckBox *_byDiscardBox = nullptr;
    cocos2d::ui::CheckBox *_selfDrawnBox = nullptr;
    cocos2d::ui::CheckBox *_fourthTileBox = nullptr;
    cocos2d::ui::CheckBox *_robKongBox = nullptr;
    cocos2d::ui::CheckBox *_replacementBox = nullptr;
    cocos2d::ui::CheckBox *_lastTileDrawnBox = nullptr;
    cocos2d::ui::CheckBox *_lastTileChaimBox = nullptr;

    cocos2d::Size _pointsAreaSize;
    cocos2d::Node *_pointsAreaNode = nullptr;
};

#endif
