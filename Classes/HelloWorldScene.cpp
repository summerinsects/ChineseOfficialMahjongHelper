#include "HelloWorldScene.h"
#include "PointsCalculator/PointsCalculatorScene.h"
#include "ScoreSheet/ScoreSheetScene.h"
#include "ScoreTable/ScoreTable.h"
#include "Other/OtherScene.h"

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

    Color4B bgColor;
    const char *normalImage, *selectedImage;
    Color3B textColor;
    if (UserDefault::getInstance()->getBoolForKey("night_mode")) {
        bgColor = Color4B(32, 37, 40, 255);
        normalImage = "source_material/btn_square_normal.png";
        selectedImage = "source_material/btn_square_highlighted.png";
        textColor = Color3B::BLACK;
    }
    else {
        bgColor = Color4B(245, 245, 245, 255);
        normalImage = "source_material/btn_square_highlighted.png";
        selectedImage = "source_material/btn_square_selected.png";
        textColor = Color3B::WHITE;
    }

    LayerColor *background = LayerColor::create(bgColor);
    this->addChild(background, -100);

    auto listener = EventListenerKeyboard::create();
    listener->onKeyReleased = [](EventKeyboard::KeyCode keyCode, Event* unused_event) {
        Director::getInstance()->end();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
        exit(0);
#endif
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    ui::Button *button = ui::Button::create(normalImage, selectedImage);
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(75.0, 32.0f));
    button->setTitleFontSize(20);
    button->setTitleColor(textColor);
    button->setTitleText("算番器");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f + 80));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(PointsCalculatorScene::createScene());
    });

    button = ui::Button::create(normalImage, selectedImage);
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(75.0, 32.0f));
    button->setTitleFontSize(20);
    button->setTitleColor(textColor);
    button->setTitleText("计分器");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f + 40));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(ScoreSheetScene::createScene());
    });

    button = ui::Button::create(normalImage, selectedImage);
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(75.0, 32.0f));
    button->setTitleFontSize(20);
    button->setTitleColor(textColor);
    button->setTitleText("番种表");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(ScoreTableScene::createScene());
    });

    button = ui::Button::create(normalImage, selectedImage);
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(75.0, 32.0f));
    button->setTitleFontSize(20);
    button->setTitleColor(textColor);
    button->setTitleText("其他");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 40));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(OtherScene::createScene());
    });

    button = ui::Button::create(normalImage, selectedImage);
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(75.0, 32.0f));
    button->setTitleFontSize(20);
    button->setTitleColor(textColor);
    button->setTitleText(UserDefault::getInstance()->getBoolForKey("night_mode") ? "日间模式" : "夜间模式");
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 80));
    button->addClickEventListener([](Ref *) {
        if (UserDefault::getInstance()->getBoolForKey("night_mode")) {
            UserDefault::getInstance()->setBoolForKey("night_mode", false);
        }
        else {
            UserDefault::getInstance()->setBoolForKey("night_mode", true);
        }
        UserDefault::getInstance()->flush();

        Director::getInstance()->replaceScene(HelloWorld::createScene());
    });

    return true;
}
