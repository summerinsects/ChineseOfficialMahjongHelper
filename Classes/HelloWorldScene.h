#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"

class HelloWorld : public cocos2d::Scene {
public:
    virtual bool init() override;

    CREATE_FUNC(HelloWorld);

private:
    void onAboutButton(cocos2d::Ref *sender);
    void requestVersion(bool manual);
};

#endif // __HELLOWORLD_SCENE_H__
