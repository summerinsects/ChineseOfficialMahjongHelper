#ifndef __BASE_LAYER__
#define __BASE_LAYER__

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class BaseLayer : public cocos2d::Layer {
public:
    bool initWithTitle(const std::string &title) {
        if (!cocos2d::Layer::init()) {
            return false;
        }

        auto listener = cocos2d::EventListenerKeyboard::create();
        listener->onKeyReleased = [](cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *unusedEvent) {
            if (keyCode == cocos2d::EventKeyboard::KeyCode::KEY_BACK) {
                cocos2d::Director::getInstance()->popScene();
            }
        };
        _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

        cocos2d::Size visibleSize = cocos2d::Director::getInstance()->getVisibleSize();
        cocos2d::Vec2 origin = cocos2d::Director::getInstance()->getVisibleOrigin();

        cocos2d::Label *tileLabel = cocos2d::Label::createWithSystemFont(title, "Arial", 20);
        this->addChild(tileLabel);
        tileLabel->setPosition(cocos2d::Vec2(origin.x + visibleSize.width * 0.5f,
            origin.y + visibleSize.height - tileLabel->getContentSize().height * 0.5f));

        cocos2d::ui::Button *backBtn = cocos2d::ui::Button::create("source_material/btn_left_white.png", "source_material/btn_left_blue.png");
        this->addChild(backBtn);
        backBtn->setScale9Enabled(true);
        backBtn->setScale(24 / backBtn->getContentSize().width);
        backBtn->setPosition(cocos2d::Vec2(origin.x + 15, origin.y + visibleSize.height - 15));
        backBtn->addClickEventListener([](cocos2d::Ref *) {
            cocos2d::Director::getInstance()->popScene();
        });

        return true;
    }

private:
    using cocos2d::Layer::init;
};

#endif
