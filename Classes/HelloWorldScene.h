#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"

class HelloWorld : public cocos2d::Layer {
public:
    static cocos2d::Scene *createScene();
    virtual bool init() override;

    CREATE_FUNC(HelloWorld);

private:
    void onAboutButton(cocos2d::Ref *sender);
    void requestVersion(bool manual);
};

#endif // __HELLOWORLD_SCENE_H__
