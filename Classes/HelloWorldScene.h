#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"

class HelloWorld : public cocos2d::Layer {
public:
    static cocos2d::Scene *createScene();
    virtual bool init() override;
    void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *unusedEvent);
    CREATE_FUNC(HelloWorld);
};

#endif // __HELLOWORLD_SCENE_H__
