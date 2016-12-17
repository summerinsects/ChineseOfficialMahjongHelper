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
    void onByDiscardButton(cocos2d::Ref *sender);
    void onSelfDrawnButton(cocos2d::Ref *sender);
    void onFourthTileButton(cocos2d::Ref *sender);
    void onRobKongButton(cocos2d::Ref *sender);
    void onReplacementButton(cocos2d::Ref *sender);
    void onLastTileButton(cocos2d::Ref *sender);

    void onFixedSetsChanged(TilePickWidget *sender);
    void onWinTileChanged(TilePickWidget *sender);

    void calculate();

    TilePickWidget *_tilePicker = nullptr;
    cocos2d::ui::EditBox *_editBox = nullptr;
    cocos2d::ui::Button *_prevalentButton[4];
    cocos2d::ui::Button *_seatButton[4];
    cocos2d::ui::Button *_byDiscardButton = nullptr;
    cocos2d::ui::Button *_selfDrawnButton = nullptr;
    cocos2d::ui::Button *_fourthTileButton = nullptr;
    cocos2d::ui::Button *_robKongButton = nullptr;
    cocos2d::ui::Button *_replacementButton = nullptr;
    cocos2d::ui::Button *_lastTileButton = nullptr;

    cocos2d::Node *_pointsAreaNode = nullptr;

    bool _maybeFourthTile = false;
    bool _hasKong = false;
    bool _maybeRobKong = false;
};

#endif
