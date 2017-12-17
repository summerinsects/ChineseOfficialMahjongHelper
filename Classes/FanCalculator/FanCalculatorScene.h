#ifndef __FAN_CALCULATOR_SCENE_H__
#define __FAN_CALCULATOR_SCENE_H__

#include "../BaseScene.h"

class TilePickWidget;
class ExtraInfoWidget;

class FanCalculatorScene : public BaseScene {
public:
    virtual bool init() override;

    CREATE_FUNC(FanCalculatorScene);

private:
    void calculate();

    TilePickWidget *_tilePicker = nullptr;
    ExtraInfoWidget *_extraInfo = nullptr;
    cocos2d::Node *_fanAreaNode = nullptr;
};

#endif
