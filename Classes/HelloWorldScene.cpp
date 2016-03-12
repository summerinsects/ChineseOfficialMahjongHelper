#include "HelloWorldScene.h"
#include "PointsCalculator/PointsCalculatorScene.h"
#include "ScoreSheet/ScoreSheetScene.h"

#pragma execution_character_set("utf-8")

USING_NS_CC;

Scene* HelloWorld::createScene() {
    auto scene = Scene::create();
    auto layer = HelloWorld::create();
    scene->addChild(layer);
    return scene;
}

bool HelloWorld::init() {
    if (!Layer::init()) {
        return false;
    }

    auto listener = EventListenerKeyboard::create();
    listener->onKeyReleased = CC_CALLBACK_2(Layer::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    ui::Button *button = ui::Button::create();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(75.0, 32.0f));
    button->setTitleFontSize(24);
    button->setTitleText("算番器");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f + 20));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(PointsCalculatorScene::createScene());
    });

    button = ui::Button::create();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(75.0, 32.0f));
    button->setTitleFontSize(24);
    button->setTitleText("计分器");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 20));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(ScoreSheetScene::createScene());
    });

    return true;
}

void HelloWorld::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* unused_event) {
    Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}