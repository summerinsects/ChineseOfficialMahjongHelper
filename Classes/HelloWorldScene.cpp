#include "HelloWorldScene.h"
#include "network/HttpClient.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "UICommon.h"
#include "utils/common.h"
#include "widget/AlertDialog.h"
#include "widget/Toast.h"
#include "widget/LoadingView.h"
#include "FanCalculator/FanCalculatorScene.h"
#include "RecordSystem/ScoreSheetScene.h"
#include "FanTable/FanTableScene.h"
#include "Other/OtherScene.h"
#include "MahjongTheory/MahjongTheoryScene.h"
#include "CompetitionSystem/CompetitionMainScene.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#define DOWNLOAD_URL "https://www.pgyer.com/comh-android"
#define QR_CODE_URL "https://www.pgyer.com/app/qrcode/comh-android"
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
#define DOWNLOAD_URL "https://www.pgyer.com/comh-ios"
#define QR_CODE_URL "https://www.pgyer.com/app/qrcode/comh-ios"
#else
#define DOWNLOAD_URL ""
#define QR_CODE_URL "https://www.pgyer.com/app/qrcode/comh-android"
#endif

USING_NS_CC;

static bool isVersionNewer(const char *version1, const char *version2);

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

    const float buttonWidth = (visibleSize.width - 16) / 3;
    const float buttonHeight = buttonWidth * 0.8f;

    ui::Button *button = UICommon::createButton();
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

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(40.0f, 25.0f));
    button->setTitleFontSize(14);
    button->setTitleText(__UTF8("关于"));
    button->setPosition(Vec2(origin.x + 23.0f, origin.y + 15.0f));
    button->addClickEventListener(std::bind(&HelloWorld::onAboutButton, this, std::placeholders::_1));

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    Sprite *sprite = Sprite::create("drawable/indicator_input_error.png");
    sprite->setScale(CC_CONTENT_SCALE_FACTOR() * 0.5f);
    button->addChild(sprite);
    sprite->setPosition(Vec2(40.0f, 25.0f));
    sprite->setVisible(UserDefault::getInstance()->getBoolForKey("has_new_version"));
    _redPointSprite = sprite;
#endif

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(40.0f, 25.0f));
    button->setTitleFontSize(14);
    button->setTitleText(__UTF8("分享"));
    button->setPosition(Vec2(origin.x + visibleSize.width - 23.0f, origin.y + 15.0f));
    button->addClickEventListener([this](Ref *) { requestQRCode(); });

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
    if (needRequest()) {
        requestVersion(false);
    }
#endif

    return true;
}

void HelloWorld::onAboutButton(cocos2d::Ref *) {
    const float width = AlertDialog::maxWidth();

    Node *rootNode = Node::create();

    Label *label = Label::createWithSystemFont(
        __UTF8("1. 本软件开源，高端玩家可下载源代码自行编译。\n")
        __UTF8("2. 本项目源代码地址：https://github.com/summerinsects/ChineseOfficialMahjongHelper\n")
        __UTF8("3. 如果觉得本软件好用，可点击「分享二维码」，分享给他人扫码下载。\n")
        __UTF8("4. 支持开源软件，欢迎打赏。")
        , "Arail", 10, Size(width, 0.0f));
    label->setColor(Color3B::BLACK);
    rootNode->addChild(label);

    const Size &labelSize = label->getContentSize();

    ui::Button *button1 = UICommon::createButton();
    button1->setScale9Enabled(true);
    button1->setContentSize(Size(65.0, 20.0f));
    button1->setTitleFontSize(12);
    button1->setTitleText(__UTF8("版本检测"));
    button1->addClickEventListener([this](Ref *) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
        requestVersion(true);
#else
        Toast::makeText(this, "当前平台不支持该操作", Toast::LENGTH_LONG)->show();
#endif
    });
    rootNode->addChild(button1);

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    if (_redPointSprite->isVisible()) {
        Sprite *sprite = Sprite::create("drawable/indicator_input_error.png");
        sprite->setScale(CC_CONTENT_SCALE_FACTOR() * 0.4f);
        button1->addChild(sprite);
        sprite->setPosition(Vec2(55.0f, 20.0f));
    }
#endif

    ui::Button *button2 = UICommon::createButton();
    button2->setScale9Enabled(true);
    button2->setContentSize(Size(65.0, 20.0f));
    button2->setTitleFontSize(12);
    button2->setTitleText(__UTF8("打赏"));
    button2->addClickEventListener([](Ref *) {
        Application::getInstance()->openURL("https://gitee.com/201103L/ChineseOfficialMahjongHelper?donate=true&&skip_mobile=true");
    });
    rootNode->addChild(button2);

    rootNode->setContentSize(Size(width, labelSize.height + 30.0f));
    button1->setPosition(Vec2(width * 0.25f, 10.0f));
    button2->setPosition(Vec2(width * 0.75f, 10.0f));
    label->setPosition(Vec2(width * 0.5f, labelSize.height * 0.5f + 30.0f));

    AlertDialog::Builder(this)
        .setTitle(__UTF8("关于"))
        .setContentNode(rootNode)
        .setCloseOnTouchOutside(false)
        .setPositiveButton(__UTF8("确定"), nullptr)
        .create()->show();
}

static void showQRCodeAlertDialog(Scene *scene, Texture2D *texture) {
    Node *rootNode = Node::create();

    ui::Button *button = UICommon::createButton();
    button->setScale9Enabled(true);
    button->setContentSize(Size(65.0, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText(__UTF8("复制链接"));
    button->addClickEventListener([scene](Ref *) {
        cw::setClipboardText(DOWNLOAD_URL);
        Toast::makeText(scene, __UTF8("下载地址已复制到剪切板"), Toast::LENGTH_LONG)->show();
    });
    rootNode->addChild(button);

    Sprite *sprite = Sprite::createWithTexture(texture);
    rootNode->addChild(sprite);

    const Size &spriteSize = sprite->getContentSize();
    rootNode->setContentSize(Size(spriteSize.width, spriteSize.height + 25.0f));
    sprite->setPosition(Vec2(spriteSize.width * 0.5f, spriteSize.height * 0.5f));
    button->setPosition(Vec2(spriteSize.width * 0.5f, spriteSize.height + 15.0f));

    AlertDialog::Builder(scene)
        .setTitle(__UTF8("分享二维码"))
        .setMessage(__UTF8("二维码iOS与Android通用"))
        .setContentNode(rootNode)
        .setCloseOnTouchOutside(false)
        .setPositiveButton(__UTF8("确定"), nullptr)
        .create()->show();
}

void HelloWorld::requestQRCode() {
    Texture2D *texture = Director::getInstance()->getTextureCache()->getTextureForKey("shared_qrcode_texture");
    if (texture != nullptr) {
        showQRCodeAlertDialog(this, texture);
        return;
    }

    const float realWidth = AlertDialog::maxWidth() * CC_CONTENT_SCALE_FACTOR();
    const int factor = static_cast<int>(realWidth / 43.0f);

    network::HttpRequest *request = new (std::nothrow) network::HttpRequest();
    request->setRequestType(network::HttpRequest::Type::GET);
    request->setUrl(Common::format("%s?pixsize=%d", QR_CODE_URL, factor * 43));

    LoadingView *loadingView = LoadingView::create();
    loadingView->showInScene(this);
    auto thiz = makeRef(this);
    request->setResponseCallback([thiz, loadingView](network::HttpClient *client, network::HttpResponse *response) {
        network::HttpClient::destroyInstance();

        loadingView->dismiss();
        if (response == nullptr) {
            return;
        }

        log("HTTP Status Code: %ld", response->getResponseCode());

        if (!response->isSucceed()) {
            log("response failed");
            log("error buffer: %s", response->getErrorBuffer());
            Toast::makeText(thiz.get(), __UTF8("获取二维码本失败"), Toast::LENGTH_LONG)->show();
            return;
        }

        std::vector<char> *buffer = response->getResponseData();
        if (buffer == nullptr) {
            Toast::makeText(thiz.get(), __UTF8("获取二维码本失败"), Toast::LENGTH_LONG)->show();
            return;
        }

        Image *image = new (std::nothrow) Image();
        image->initWithImageData(reinterpret_cast<const unsigned char *>(buffer->data()), static_cast<ssize_t>(buffer->size()));
        Texture2D *texture = Director::getInstance()->getTextureCache()->addImage(image, "shared_qrcode_texture");
        image->release();
        showQRCodeAlertDialog(thiz.get(), texture);
    });

    network::HttpClient::getInstance()->send(request);
    request->release();
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)

bool HelloWorld::needRequest() const {
    UserDefault *userDefault = UserDefault::getInstance();
    userDefault->deleteValueForKey("not_notify");

    // 有新版本
    if (userDefault->getBoolForKey("has_new_version")) {
        // 如果未选中明天提醒，则检测
        // 如果选中明天提醒，超过一天时，检测
        if (!userDefault->getBoolForKey("notify_tomorrow")) {
            return true;
        }
    }

    // 距离上次检测有间隔一天再检测
    time_t lastTime = static_cast<time_t>(atoll(userDefault->getStringForKey("last_request_time").c_str()));
    time_t now = time(nullptr);
    struct tm tma = *localtime(&lastTime);
    struct tm tmb = *localtime(&now);
    return (tma.tm_year != tmb.tm_year || tma.tm_mon != tmb.tm_mon || tma.tm_mday != tmb.tm_mday);
}

void HelloWorld::requestVersion(bool manual) {
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
                Toast::makeText(this, __UTF8("获取最新版本失败"), Toast::LENGTH_LONG)->show();
            }
            return;
        }

        std::vector<char> *buffer = response->getResponseData();
        if (!checkVersion(buffer, manual)) {
            if (manual) {
                Toast::makeText(this, __UTF8("获取最新版本失败"), Toast::LENGTH_LONG)->show();
            }
        }
    });

    network::HttpClient::getInstance()->send(request);
    request->release();
}

bool HelloWorld::checkVersion(const std::vector<char> *buffer, bool manual) {
    if (buffer == nullptr) {
        return false;
    }

    try {
        std::string str(buffer->begin(), buffer->end());
        rapidjson::Document doc;
        doc.Parse<0>(str.c_str());
        if (doc.HasParseError() || !doc.IsObject()) {
            return false;
        }

        rapidjson::Value::ConstMemberIterator it = doc.FindMember("tag_name");
        if (it == doc.MemberEnd() || !it->value.IsString()) {
            return false;
        }
        std::string tag = it->value.GetString();
        bool hasNewVersion = isVersionNewer(tag.c_str() + 1, Application::getInstance()->getVersion().c_str());

        UserDefault *userDefault = UserDefault::getInstance();
        userDefault->setBoolForKey("has_new_version", hasNewVersion);
        userDefault->setStringForKey("last_request_time", std::to_string(time(nullptr)));
        userDefault->setBoolForKey("notify_tomorrow", false);

        if (!hasNewVersion) {
            _redPointSprite->setVisible(false);
            if (manual) {
                Toast::makeText(this, __UTF8("已经是最新版本"), Toast::LENGTH_LONG)->show();
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

            Label *label = Label::createWithSystemFont(__UTF8("今日之内不再提示"), "Arial", 12);
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
            .setTitle(__UTF8("检测到新版本"))
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
            .setMessage(Common::format(__UTF8("%s，是否下载？\n\n%s"), tag.c_str(), body.c_str()))
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
            .setMessage(Common::format(__UTF8("%s，是否下载？\n提取密码xyg\n\n%s"), tag.c_str(), body.c_str()))
#endif
            .setContentNode(rootNode)
            .setCloseOnTouchOutside(false)
            .setNegativeButton(__UTF8("取消"), [checkBox](AlertDialog *, int) {
                if (checkBox != nullptr && checkBox->isSelected()) {
                    UserDefault::getInstance()->setBoolForKey("notify_tomorrow", true);
                }
                return true;
            })
            .setPositiveButton(__UTF8("更新"), [](AlertDialog *, int) {
                Application::getInstance()->openURL(DOWNLOAD_URL);
                return true;
            })
            .create()->show();

        return true;
    }
    catch (std::exception &e) {
        CCLOG("%s %s", __FUNCTION__, e.what());
    }

    return false;
}

#endif

static bool isVersionNewer(const char *version1, const char *version2) {
    int major1, minor1, build1, revision1;
    if (sscanf(version1, "%d.%d.%d.%d", &major1, &minor1, &build1, &revision1) != 4) {
        return false;
    }

    int major2, minor2, build2, revision2;
    if (sscanf(version2, "%d.%d.%d.%d", &major2, &minor2, &build2, &revision2) != 4) {
        return false;
    }

    if (major1 > major2) {
        return true;
    }
    if (major1 == major2) {
        if (minor1 > minor2) {
            return true;
        }
        if (minor1 == minor2) {
            if (build1 > build2) {
                return true;
            }
            if (build1 == build2) {
                if (revision1 > revision2) {
                    return true;
                }
            }
        }
    }

    return false;
}
