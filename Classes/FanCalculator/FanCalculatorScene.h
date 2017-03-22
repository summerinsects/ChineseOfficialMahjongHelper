#ifndef __FAN_CALCULATOR_SCENE_H__
#define __FAN_CALCULATOR_SCENE_H__

#include "../BaseLayer.h"

class TilePickWidget;
class ExtraInfoWidget;

class FanCalculatorScene : public BaseLayer {
public:
    static cocos2d::Scene *createScene();

    virtual bool init() override;

    CREATE_FUNC(FanCalculatorScene);

private:
    void onFixedPacksChanged();
    void onWinTileChanged();

    void calculate();

    TilePickWidget *_tilePicker = nullptr;
    ExtraInfoWidget *_extraInfo = nullptr;
    cocos2d::Node *_fanAreaNode = nullptr;
};

#endif
