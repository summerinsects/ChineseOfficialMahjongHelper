#ifndef _LEFT_SIDE_MENU_H_
#define _LEFT_SIDE_MENU_H_

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class LeftSideMenu : public cocos2d::Layer {
public:
    CREATE_FUNC(LeftSideMenu);
    virtual bool init() override;

    void dismiss() {
        this->removeFromParent();
    }

private:

};

#endif
