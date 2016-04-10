#ifndef __SCORE_TABLE_H__
#define __SCORE_TABLE_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class ScoreTableScene : public cocos2d::Layer {
public:
    static cocos2d::Scene *createScene();

    virtual bool init() override;

    virtual void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *unusedEvent) override;
    CREATE_FUNC(ScoreTableScene);

private:
};

#endif
