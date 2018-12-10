#include "HelloWorldScene.h"
#include "UICommon.h"
#include "UIColors.h"
#include "utils/common.h"
#include "widget/AlertDialog.h"
#include "widget/LoadingView.h"
#include "FanCalculator/FanCalculatorScene.h"
#include "RecordSystem/ScoreSheetScene.h"
#include "FanTable/FanTableScene.h"
#include "Other/OtherScene.h"
#include "MahjongTheory/MahjongTheoryScene.h"
#include "CompetitionSystem/CompetitionMainScene.h"
#include "MainMenu/LeftSideMenu.h"

USING_NS_CC;

extern void UpgradeRecordInFile(const char *file);
extern void UpgradeHistoryRecords(const char *file);

bool HelloWorld::init() {
    if (UNLIKELY(!Scene::init())) {
        return false;
    }

    UserDefault::getInstance()->deleteValueForKey("night_mode");
    UserDefault::getInstance()->deleteValueForKey("notify_tomorrow");
    UserDefault::getInstance()->deleteValueForKey("has_new_version");

    LayerColor *background = LayerColor::create(COLOR4B_BG);
    this->addChild(background, -100);

    auto listener = EventListenerKeyboard::create();
    listener->onKeyReleased = [this](EventKeyboard::KeyCode keyCode, Event *unusedEvent) {
        CC_UNUSED_PARAM(keyCode);
        CC_UNUSED_PARAM(unusedEvent);
        AlertDialog::Builder(this)
            .setTitle(__UTF8("提示"))
            .setMessage(__UTF8("是否确定退出国标小助手？"))
            .setNegativeButton(__UTF8("取消"), nullptr)
            .setPositiveButton(__UTF8("确定"), [](AlertDialog *, int) {
            Director::getInstance()->end();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
            exit(0);
#endif
            return true;
        }).create()->show();
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

#if ENABLE_LOGO
    for (int i = 0; i < 3; ++i) {
        Sprite *sprite = Sprite::create("xyg.png");
        this->addChild(sprite);
        sprite->setOpacity(0x10);
        sprite->setRotation(-45);
        sprite->setScale(256 / sprite->getContentSize().width);
        sprite->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f + 200 * (1 - i)));
    }
#endif

    // 菜单按钮
    ui::Button *button = ui::Button::create("icon/menu.png");
    this->addChild(button);
    button->setScale(20 / button->getContentSize().width);
    button->setColor(Color3B(51, 204, 255));
    button->setPosition(cocos2d::Vec2(origin.x + 15.0f, origin.y + visibleSize.height - 15.0f));
    button->addClickEventListener([this](cocos2d::Ref *) {
        LeftSideMenu::create(this)->show();
    });

    const float buttonWidth = (visibleSize.width - 16) / 3;
    const float buttonHeight = buttonWidth * 0.8f;

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText(__UTF8("算番器"));
    button->setPosition(Vec2(origin.x + buttonWidth * 0.5f + 4.0f, origin.y + visibleSize.height * 0.5f + buttonHeight * 0.5f + 12.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(FanCalculatorScene::create());
    });

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText(__UTF8("计分器"));
    button->setPosition(Vec2(origin.x + buttonWidth * 1.5f + 8.0f, origin.y + visibleSize.height * 0.5f + buttonHeight * 0.5f + 12.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(ScoreSheetScene::create());
    });

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText(__UTF8("番种表"));
    button->setPosition(Vec2(origin.x + buttonWidth * 2.5f + 12.0f, origin.y + visibleSize.height * 0.5f + buttonHeight * 0.5f + 12.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(FanTableScene::create());
    });

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText(__UTF8("牌理"));
    button->setPosition(Vec2(origin.x + buttonWidth * 0.5f + 4.0f, origin.y + visibleSize.height * 0.5f - buttonHeight * 0.5f + 8.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(MahjongTheoryScene::create());
    });

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText(__UTF8("比赛"));
    button->setPosition(Vec2(origin.x + buttonWidth * 1.5f + 8.0f, origin.y + visibleSize.height * 0.5f - buttonHeight * 0.5f + 8.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(CompetitionMainScene::create());
    });

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText(__UTF8("其他"));
    button->setPosition(Vec2(origin.x + buttonWidth * 2.5f + 12.0f, origin.y + visibleSize.height * 0.5f - buttonHeight * 0.5f + 8.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(OtherScene::create());
    });

#if 0
    label = Label::createWithSystemFont(Common::format("{{%.2f, %.2f}, {%.2f, %.2f}}", origin.x, origin.y, visibleSize.width, visibleSize.height),
        "Arial", 10);
    label->setTextColor(C4B_BLACK);
    this->addChild(label);
    label->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + 15.0f));
#endif

    upgradeDataIfNecessary();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    LeftSideMenu::checkVersion(this, false);
#endif

    return true;
}

void HelloWorld::upgradeDataIfNecessary() {
    std::string version = Application::getInstance()->getVersion();
    std::string dv = UserDefault::getInstance()->getStringForKey("data_version");
    if (dv.empty() || Common::compareVersion(version.c_str(), dv.c_str())) {
        LoadingView *loadingView = LoadingView::create();
        loadingView->showInScene(this);
        auto thiz = makeRef(this);

        AsyncTaskPool::getInstance()->enqueue(AsyncTaskPool::TaskType::TASK_IO, [thiz, loadingView, version](void *) {
            loadingView->dismiss();
            UserDefault::getInstance()->setStringForKey("data_version", version);
        }, nullptr, []() {
            const std::string path = FileUtils::getInstance()->getWritablePath();
            UpgradeRecordInFile((path + "record.json").c_str());
            UpgradeHistoryRecords((path + "history_record.json").c_str());
        });
    }
}
