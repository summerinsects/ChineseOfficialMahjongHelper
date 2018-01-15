#include "HelloWorldScene.h"
#include "network/HttpClient.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "UICommon.h"
#include "utils/common.h"
#include "widget/AlertDialog.h"
#include "widget/Toast.h"
#include "FanCalculator/FanCalculatorScene.h"
#include "RecordSystem/ScoreSheetScene.h"
#include "FanTable/FanTableScene.h"
#include "Other/OtherScene.h"
#include "MahjongTheory/MahjongTheoryScene.h"
#include "CompetitionSystem/CompetitionMainScene.h"

USING_NS_CC;

bool HelloWorld::init() {
    if (UNLIKELY(!Scene::init())) {
        return false;
    }

    UserDefault::getInstance()->deleteValueForKey("night_mode");

    LayerColor *background = LayerColor::create(COLOR4B_BG);
    this->addChild(background, -100);

    auto listener = EventListenerKeyboard::create();
    listener->onKeyReleased = [this](EventKeyboard::KeyCode keyCode, Event *unusedEvent) {
        CC_UNUSED_PARAM(keyCode);
        CC_UNUSED_PARAM(unusedEvent);
        AlertDialog::Builder(this)
            .setTitle("提示")
            .setMessage("是否确定退出国标小助手？")
            .setNegativeButton("取消", nullptr)
            .setPositiveButton("确定", [](AlertDialog *, int) {
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

    const float buttonWidth = (visibleSize.width - 16) / 3;
    const float buttonHeight = buttonWidth * 0.8f;

    ui::Button *button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText("算番器");
    button->setPosition(Vec2(origin.x + buttonWidth * 0.5f + 4.0f, origin.y + visibleSize.height * 0.5f + buttonHeight * 0.5f + 12.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(FanCalculatorScene::create());
    });

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText("计分器");
    button->setPosition(Vec2(origin.x + buttonWidth * 1.5f + 8.0f, origin.y + visibleSize.height * 0.5f + buttonHeight * 0.5f + 12.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(ScoreSheetScene::create());
    });

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText("番种表");
    button->setPosition(Vec2(origin.x + buttonWidth * 2.5f + 12.0f, origin.y + visibleSize.height * 0.5f + buttonHeight * 0.5f + 12.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(FanTableScene::create());
    });

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText("牌理");
    button->setPosition(Vec2(origin.x + buttonWidth * 0.5f + 4.0f, origin.y + visibleSize.height * 0.5f - buttonHeight * 0.5f + 8.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(MahjongTheoryScene::create());
    });

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText("比赛");
    button->setPosition(Vec2(origin.x + buttonWidth * 1.5f + 8.0f, origin.y + visibleSize.height * 0.5f - buttonHeight * 0.5f + 8.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(CompetitionMainScene::create());
    });

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText("其他");
    button->setPosition(Vec2(origin.x + buttonWidth * 2.5f + 12.0f, origin.y + visibleSize.height * 0.5f - buttonHeight * 0.5f + 8.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(OtherScene::create());
    });

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(40.0f, 25.0f));
    button->setTitleFontSize(14);
    button->setTitleText("关于");
    button->setPosition(Vec2(origin.x + 23.0f, origin.y + 15.0f));
    button->addClickEventListener(std::bind(&HelloWorld::onAboutButton, this, std::placeholders::_1));

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    Sprite *sprite = Sprite::create("drawable/indicator_input_error.png");
    sprite->setScale(CC_CONTENT_SCALE_FACTOR() * 0.5f);
    button->addChild(sprite);
    sprite->setPosition(Vec2(40.0f, 25.0f));
    sprite->setVisible(false);
    _redPointSprite = sprite;
#endif

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(40.0f, 25.0f));
    button->setTitleFontSize(14);
    button->setTitleText("捐赠");
    button->setPosition(Vec2(origin.x + visibleSize.width - 23.0f, origin.y + 15.0f));
    button->addClickEventListener([](Ref *) {
        Application::getInstance()->openURL("https://gitee.com/201103L/ChineseOfficialMahjongHelper?donate=true&&skip_mobile=true");
    });

    std::string version = Application::getInstance()->getVersion();
    Label *label = Label::createWithSystemFont(
        Common::format("v%s\n%s", version.c_str(), "Built  " __DATE__ "  " __TIME__), "Arial", 10);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAlignment(TextHAlignment::CENTER);
    label->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + 15.0f));

#if 0
    label = Label::createWithSystemFont(Common::format("{{%.2f, %.2f}, {%.2f, %.2f}}", origin.x, origin.y, visibleSize.width, visibleSize.height),
        "Arial", 10);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + 35.0f));
#endif

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    time_t lastTime = static_cast<time_t>(atoll(UserDefault::getInstance()->getStringForKey("not_notify").c_str()));
    time_t now = time(nullptr);

    struct tm tma = *localtime(&lastTime);
    struct tm tmb = *localtime(&now);
    if (tma.tm_year != tmb.tm_year || tma.tm_mon != tmb.tm_mon || tma.tm_mday != tmb.tm_mday) {
        requestVersion(false);
    }
#endif

    return true;
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
static void shareApplication() {
    const float width = AlertDialog::maxWidth();

    std::string version = Application::getInstance()->getVersion();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    std::string str = "<div style=\"word-break:break-all\">https://www.pgyer.com/awtU</div>";
#else
    std::string str = "<div style=\"word-break:break-all\">https://www.pgyer.com/1lMK</div>";
#endif
    Node *rootNode = Node::create();
    experimental::ui::WebView *webView = experimental::ui::WebView::create();
    webView->setContentSize(Size(width, 80.0f));
    webView->setBackgroundTransparent();
    webView->setScalesPageToFit(true);
    webView->setOnEnterCallback(std::bind(&experimental::ui::WebView::loadHTMLString, webView, std::ref(str), ""));
    rootNode->addChild(webView);
    webView->setPosition(Vec2(width * 0.5f, 40.0f));
    rootNode->setContentSize(Size(width, 80.0f));

    AlertDialog::Builder(Director::getInstance()->getRunningScene())
        .setTitle("下载地址")
        .setContentNode(rootNode)
        .setCloseOnTouchOutside(false)
        .setPositiveButton("确定", nullptr)
        .create()->show();
}
#endif

void HelloWorld::onAboutButton(cocos2d::Ref *) {
    const float width = AlertDialog::maxWidth();

    Node *rootNode = Node::create();

    Label *label = Label::createWithSystemFont(
        "1. 本软件开源，高端玩家可下载源代码自行编译。\n"
        "2. 本项目源代码地址：https://github.com/summerinsects/ChineseOfficialMahjongHelper\n"
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
        "3. 如果觉得本软件好用，可点击「下载地址」获取下载链接，分享给他人。"
#endif
        , "Arail", 10, Size(width, 0.0f));
    label->setColor(Color3B::BLACK);
    rootNode->addChild(label);

    const Size &labelSize = label->getContentSize();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    ui::Button *button1 = UICommon::createButton();
    button1->setScale9Enabled(true);
    button1->setContentSize(Size(65.0, 20.0f));
    button1->setTitleFontSize(12);
    button1->setTitleText("检测新版本");
    button1->addClickEventListener([this](Ref *) { requestVersion(true); });
    rootNode->addChild(button1);

    if (_redPointSprite->isVisible()) {
        Sprite *sprite = Sprite::create("drawable/indicator_input_error.png");
        sprite->setScale(CC_CONTENT_SCALE_FACTOR() * 0.4f);
        button1->addChild(sprite);
        sprite->setPosition(Vec2(65.0f, 20.0f));
    }

    ui::Button *button2 = UICommon::createButton();
    button2->setScale9Enabled(true);
    button2->setContentSize(Size(65.0, 20.0f));
    button2->setTitleFontSize(12);
    button2->setTitleText("下载地址");
    button2->addClickEventListener([](Ref *) {
        shareApplication();
    });
    rootNode->addChild(button2);

    rootNode->setContentSize(Size(width, labelSize.height + 30.0f));
    button1->setPosition(Vec2(width * 0.25f, 10.0f));
    button2->setPosition(Vec2(width * 0.75f, 10.0f));
    label->setPosition(Vec2(width * 0.5f, labelSize.height * 0.5f + 30.0f));
#else
    rootNode->setContentSize(Size(width, labelSize.height));
    label->setPosition(Vec2(width * 0.5f, labelSize.height * 0.5f));
#endif

    AlertDialog::Builder(this)
        .setTitle("关于")
        .setContentNode(rootNode)
        .setCloseOnTouchOutside(false)
        .setPositiveButton("确定", nullptr)
        .create()->show();
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
void HelloWorld::requestVersion(bool manual) {
#if 1
    static bool checking = false;
    if (checking) {
        return;
    }

    checking = true;

    network::HttpRequest *request = new (std::nothrow) network::HttpRequest();
    request->setRequestType(network::HttpRequest::Type::GET);
    request->setUrl("https://api.github.com/repos/summerinsects/ChineseOfficialMahjongHelper/releases/latest");

    request->setResponseCallback([this, manual](network::HttpClient *client, network::HttpResponse *response) {
        network::HttpClient::destroyInstance();

        checking = false;

        if (response == nullptr) {
            return;
        }

        log("HTTP Status Code: %ld", response->getResponseCode());

        if (!response->isSucceed()) {
            log("response failed");
            log("error buffer: %s", response->getErrorBuffer());
            if (manual) {
                Toast::makeText(this, "获取最新版本失败", Toast::LENGTH_LONG)->show();
            }
            return;
        }

        std::vector<char> *buffer = response->getResponseData();
        if (!checkVersion(buffer, manual)) {
            if (manual) {
                Toast::makeText(this, "获取最新版本失败", Toast::LENGTH_LONG)->show();
            }
        }
    });

    network::HttpClient::getInstance()->sendImmediate(request);
    request->release();
#else
    std::string str =
    "{"
    "  \"tag_name\": \"v1.2.13\","
    "  \"body\": \"1. 更换了新的图片\\r\\n2. 记分器的记录界面标记番种增加了「最近使用」\\r\\n3. 其他细节优化\""
    "}";
    std::vector<char> buffer(str.begin(), str.end());
    checkVersion(&buffer, false);
#endif
}
#endif

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
bool HelloWorld::checkVersion(const std::vector<char> *buffer, bool manual) {
    if (buffer == nullptr) {
        return false;
    }

    try {
        do {
            std::string str(buffer->begin(), buffer->end());
            rapidjson::Document doc;
            doc.Parse<0>(str.c_str());
            if (doc.HasParseError() || !doc.IsObject()) {
                break;
            }

            rapidjson::Value::ConstMemberIterator it = doc.FindMember("tag_name");
            if (it == doc.MemberEnd() || !it->value.IsString()) {
                break;
            }
            std::string tag = it->value.GetString();
            int major1, minor1, point1;
            if (sscanf(tag.c_str(), "v%d.%d.%d", &major1, &minor1, &point1) != 3) {
                break;
            }

            std::string version = Application::getInstance()->getVersion();
            int a, b, c;
            if (sscanf(version.c_str(), "%d.%d.%d", &a, &b, &c) != 3) {
                break;
            }

            bool hasNewVersion = false;
            if (major1 > a) {
                hasNewVersion = true;
            }
            else if (major1 == a) {
                if (minor1 > b) {
                    hasNewVersion = true;
                }
                else if (minor1 == b) {
                    if (point1 > c) {
                        hasNewVersion = true;
                    }
                }
            }

            if (!hasNewVersion) {
                if (manual) {
                    Toast::makeText(this, "已经是最新版本", Toast::LENGTH_LONG)->show();
                }
                return true;
            }

            _redPointSprite->setVisible(true);

            Node *rootNode = Node::create();
            ui::CheckBox *checkBox = nullptr;
            if (!manual) {
                checkBox = UICommon::createCheckBox();
                rootNode->addChild(checkBox);
                checkBox->setZoomScale(0.0f);
                checkBox->ignoreContentAdaptWithSize(false);
                checkBox->setContentSize(Size(20.0f, 20.0f));
                checkBox->setPosition(Vec2(10.0f, 10.0f));

                Label *label = Label::createWithSystemFont("今日之内不再提示", "Arial", 12);
                label->setColor(Color3B::BLACK);
                rootNode->addChild(label);
                label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
                label->setPosition(Vec2(25.0f, 10.0f));

                rootNode->setContentSize(Size(25.0f + label->getContentSize().width, 20.0f));
            }

            it = doc.FindMember("body");
            std::string body;
            if (it != doc.MemberEnd() && it->value.IsString()) {
                body = it->value.GetString();
            }

            AlertDialog::Builder(this)
                .setTitle("检测到新版本")
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
                .setMessage(Common::format("%s，是否下载？\n\n%s", tag.c_str(), body.c_str()))
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
                .setMessage(Common::format("%s，是否下载？\n提取密码xyg\n\n%s", tag.c_str(), body.c_str()))
#endif
                .setContentNode(rootNode)
                .setCloseOnTouchOutside(false)
                .setNegativeButton("取消", [checkBox](AlertDialog *, int) {
                    if (checkBox != nullptr && checkBox->isSelected()) {
                        UserDefault::getInstance()->setStringForKey("not_notify", std::to_string(time(nullptr)));
                    }
                    return true;
                })
                .setPositiveButton("更新", [](AlertDialog *, int) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
                    Application::getInstance()->openURL("https://www.pgyer.com/awtU");
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
                    Application::getInstance()->openURL("https://www.pgyer.com/1lMK");
#endif
                    return true;
                })
                .create()->show();
            return true;
        } while (0);
    }
    catch (std::exception &e) {
        CCLOG("%s %s", __FUNCTION__, e.what());
    }

    return false;
}
#endif
