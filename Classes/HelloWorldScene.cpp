#include "HelloWorldScene.h"

#pragma execution_character_set("utf-8")
#include "PointsCalculatorScene.h"
USING_NS_CC;

Scene* HelloWorld::createScene() {
    return PointsCalculatorScene::createScene();
    // 'scene' is an autorelease object
    auto scene = Scene::create();
    
    // 'layer' is an autorelease object
    auto layer = HelloWorld::create();

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool HelloWorld::init() {
    //////////////////////////////
    // 1. super init first
    if (!Layer::init()) {
        return false;
    }
    
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    Label *tileLabel = Label::createWithSystemFont("麻将竞赛成绩记录表", "Arial", 26);
    this->addChild(tileLabel);
    tileLabel->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height - tileLabel->getContentSize().height * 0.5f));


    for (int i = 0; i < 7; ++i) {
        LayerColor *vertLine = LayerColor::create(Color4B(255, 255, 255, 255), 3, 300);
        this->addChild(vertLine);
        vertLine->setPosition(Vec2(origin.x + 10 + 60 * i,
            origin.y + visibleSize.height * 0.5f));
    }
    for (int i = 0; i < 22; ++i) {

        //LayerColor *
    }

    return true;
}


void HelloWorld::menuCloseCallback(Ref* pSender) {
    Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}

void HelloWorld::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* unused_event) {
    MessageBox("123", "456");
}