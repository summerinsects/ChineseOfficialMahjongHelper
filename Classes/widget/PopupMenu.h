#ifndef _POPUP_MENU_H_
#define _POPUP_MENU_H_

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "../cocos-wheels/CWCommon.h"

class PopupMenu : public cocos2d::Layer {
public:
    CREATE_FUNC_WITH_PARAM_4(PopupMenu, cocos2d::Scene *, scene, const std::vector<std::string> &, menuTexts, const cocos2d::Vec2 &, basePos, const cocos2d::Vec2 &, anchorPoint);
    bool init(cocos2d::Scene *scene, const std::vector<std::string> &menuTexts, const cocos2d::Vec2 &basePos, const cocos2d::Vec2 &anchorPoint);

    void setMenuItemCallback(std::function<void (PopupMenu *, size_t)> &&onMenuItemCallback) { _onMenuItemCallback.swap(onMenuItemCallback); }

    void show() {
        _scene->addChild(this, 100);
        _scene = nullptr;
    }

    void dismiss() {
        this->removeFromParent();
    }

private:
    void onMenuButton(cocos2d::Ref *sender);

    cocos2d::RefPtr<cocos2d::Scene> _scene;
    std::function<void (PopupMenu *, size_t)> _onMenuItemCallback;
};

#endif
