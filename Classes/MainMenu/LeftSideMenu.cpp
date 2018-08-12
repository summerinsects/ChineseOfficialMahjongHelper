#include "LeftSideMenu.h"
#include "../utils/compiler.h"
#include "../UIColors.h"

USING_NS_CC;

bool LeftSideMenu::init() {
    if (UNLIKELY(!Layer::init())) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    const float maxWidth = visibleSize.width * 0.4f;

    // 监听返回键
    EventListenerKeyboard *keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyReleased = [this](EventKeyboard::KeyCode keyCode, Event *event) {
        if (keyCode == EventKeyboard::KeyCode::KEY_BACK) {
            event->stopPropagation();
            this->dismiss();
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

    // 遮罩
    this->addChild(LayerColor::create(Color4B(0, 0, 0, 127)));

    // 背景
    LayerColor *background = LayerColor::create(Color4B(255, 255, 255, 245));
    this->addChild(background);
    background->setPosition(origin);

    background->setContentSize(Size(maxWidth, visibleSize.height));


    // 触摸监听，点击background以外的部分按按下取消键处理
    EventListenerTouchOneByOne *touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [this, background](Touch *touch, Event *event) {
        Vec2 pos = this->convertTouchToNodeSpace(touch);
        if (background->getBoundingBox().containsPoint(pos)) {
            return true;
        }
        event->stopPropagation();
        this->dismiss();
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
    _touchListener = touchListener;

    return true;
}
