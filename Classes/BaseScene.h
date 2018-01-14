#ifndef __BASE_SCENE__
#define __BASE_SCENE__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "utils/common.h"
#include "cocos-wheels/CWCommon.h"

#define COLOR4B_BG          cocos2d::Color4B(245, 245, 245, 255)

#define ENABLE_LOGO 1

class BaseScene : public cocos2d::Scene {
public:
    bool initWithTitle(const std::string &title) {
        if (UNLIKELY(!cocos2d::Scene::init())) {
            return false;
        }

        // 背景色
        cocos2d::LayerColor *background = cocos2d::LayerColor::create(COLOR4B_BG);
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

#if ENABLE_LOGO
        for (int i = 0; i < 3; ++i) {
            cocos2d::Sprite *sprite = cocos2d::Sprite::create("xyg.png");
            this->addChild(sprite);
            sprite->setOpacity(0x10);
            sprite->setRotation(-45);
            sprite->setScale(256 / sprite->getContentSize().width);
            sprite->setPosition(cocos2d::Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f + 200.0f * (1 - i)));
        }
#endif

        // 导航栏
        cocos2d::LayerColor *navigation = cocos2d::LayerColor::create(cocos2d::Color4B(51, 204, 255, 230), visibleSize.width, 30.0f);
        this->addChild(navigation);
        navigation->setPosition(cocos2d::Vec2(origin.x, origin.y + visibleSize.height - 30.0f));

        // 标题
        cocos2d::Label *titleLabel = cocos2d::Label::createWithSystemFont(title, "Arial", 18);
        this->addChild(titleLabel);
        titleLabel->setPosition(cocos2d::Vec2(origin.x + visibleSize.width * 0.5f,
            origin.y + visibleSize.height - 15));
        cw::trimLabelStringWithEllipsisToFitWidth(titleLabel, visibleSize.width - 64.0f);

        // 返回按钮
        cocos2d::ui::Button *backBtn = cocos2d::ui::Button::create("source_material/btn_left_white.png");
        this->addChild(backBtn);
        backBtn->setScale(24 / backBtn->getContentSize().width);
        backBtn->setPosition(cocos2d::Vec2(origin.x + 15.0f, origin.y + visibleSize.height - 15.0f));
        backBtn->addClickEventListener([](cocos2d::Ref *) {
            cocos2d::Director::getInstance()->popScene();
        });

        return true;
    }
};

#endif
