#ifndef __SCORE_DEFINITION_H__
#define __SCORE_DEFINITION_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class ScoreDefinitionScene : public cocos2d::Layer {
public:
    static cocos2d::Scene *createScene(size_t idx);

    bool initWithIndex(size_t idx);

    virtual void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *unusedEvent) override;
private:
};

#endif
