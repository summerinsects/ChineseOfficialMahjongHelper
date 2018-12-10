#ifndef _LEFT_SIDE_MENU_H_
#define _LEFT_SIDE_MENU_H_

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "../cocos-wheels/CWCommon.h"

class LeftSideMenu : public cocos2d::Layer {
public:
    CREATE_FUNC_WITH_PARAM_1(LeftSideMenu, cocos2d::Scene *, scene);
    bool init(cocos2d::Scene *scene);

    void show() {
        _scene->addChild(this, 100);
    }

    void dismiss() {
        this->removeFromParent();
    }

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    static void checkVersion(cocos2d::Scene *scene, bool manual);
#endif

private:
    cocos2d::Scene *_scene = nullptr;

    void onSettingButton(cocos2d::Ref *sender);
    void onSharedButton(cocos2d::Ref *sender);
    void onDonationButton(cocos2d::Ref *sender);
    void onUpdateLogButton(cocos2d::Ref *sender);
    void onVersionCheckButton(cocos2d::Ref *sender);
    void onExitButton(cocos2d::Ref *sender);

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    static bool _checkVersion(cocos2d::Scene *scene, bool manual, const std::vector<char> *buffer);
#endif
};

#endif
