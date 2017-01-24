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

        bool nightMode = cocos2d::UserDefault::getInstance()->getBoolForKey("night_mode");

        // 背景色
        cocos2d::LayerColor *background = cocos2d::LayerColor::create(
            nightMode ? cocos2d::Color4B(32, 37, 40, 255) : cocos2d::Color4B(240, 240, 240, 255));
        this->addChild(background, -100);

        // 监听返回键
        auto listener = cocos2d::EventListenerKeyboard::create();
        listener->onKeyReleased = [](cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *event) {
            if (keyCode == cocos2d::EventKeyboard::KeyCode::KEY_BACK) {
                event->stopPropagation();
                cocos2d::Director::getInstance()->popScene();
            }
        };
        _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

        cocos2d::Size visibleSize = cocos2d::Director::getInstance()->getVisibleSize();
        cocos2d::Vec2 origin = cocos2d::Director::getInstance()->getVisibleOrigin();

        // 导航栏
        if (!nightMode) {
            cocos2d::LayerColor *navigation = cocos2d::LayerColor::create(cocos2d::Color4B(51, 204, 255, 255), visibleSize.width, 30);
            this->addChild(navigation);
            navigation->setPosition(cocos2d::Vec2(origin.x, origin.y + visibleSize.height - 30));
        }

        // 标题
        cocos2d::Label *titleLabel = cocos2d::Label::createWithSystemFont(title, "Arial", 18);
        this->addChild(titleLabel);
        titleLabel->setPosition(cocos2d::Vec2(origin.x + visibleSize.width * 0.5f,
            origin.y + visibleSize.height - 15));

        // 返回按钮
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
