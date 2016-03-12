#ifndef _POINTS_CALCULATOR_SCENE_H_
#define _POINTS_CALCULATOR_SCENE_H_

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class PointsCalculatorScene : public cocos2d::Layer {
public:
    static cocos2d::Scene *createScene();

    virtual bool init() override;

    virtual void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *unusedEvent) override;

    CREATE_FUNC(PointsCalculatorScene);

private:
    void calculate();

    cocos2d::ui::EditBox *_editBox = nullptr;
    cocos2d::ui::Button *_prevalentButton[4];
    cocos2d::ui::Button *_seatButton[4];
    cocos2d::ui::Button *_byDiscardButton = nullptr;
    cocos2d::ui::Button *_selfDrawnButton = nullptr;
    cocos2d::ui::Button *_fourthTileButton = nullptr;
    cocos2d::ui::Button *_robKongButton = nullptr;
    cocos2d::ui::Button *_replacementButton = nullptr;
    cocos2d::ui::Button *_lastTileDrawnButton = nullptr;
    cocos2d::ui::Button *_lastTileClaimButton = nullptr;

    float _pointsAreaTop;
    float _pointsAreaBottom;
    cocos2d::Node *_pointsAreaNode = nullptr;
};

#endif
