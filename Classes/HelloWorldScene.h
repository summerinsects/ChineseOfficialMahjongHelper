#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"

class HelloWorld : public cocos2d::Scene {
public:
    virtual bool init() override;

    CREATE_FUNC(HelloWorld);

private:
    void onAboutButton(cocos2d::Ref *sender);
    void onSettingButton(cocos2d::Ref *sender);

    void requestQRCode();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    bool needRequest() const;
    void requestVersion(bool manual);
    bool checkVersion(const std::vector<char> *buffer, bool manual);

    cocos2d::Sprite *_redPointSprite = nullptr;
#endif

};

#endif // __HELLOWORLD_SCENE_H__
