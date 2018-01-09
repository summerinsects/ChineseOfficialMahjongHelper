#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"

class HelloWorld : public cocos2d::Scene {
public:
    virtual bool init() override;

    CREATE_FUNC(HelloWorld);

private:
    void onAboutButton(cocos2d::Ref *sender);

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    void requestVersion(bool manual);
    bool checkVersion(const std::vector<char> *buffer, bool manual);
#endif

};

#endif // __HELLOWORLD_SCENE_H__
