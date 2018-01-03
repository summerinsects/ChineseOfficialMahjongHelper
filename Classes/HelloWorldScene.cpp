#include "HelloWorldScene.h"
#include "network/HttpClient.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "utils/common.h"
#include "widget/AlertView.h"
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
    listener->onKeyReleased = [](EventKeyboard::KeyCode keyCode, Event *unusedEvent) {
        CC_UNUSED_PARAM(keyCode);
        CC_UNUSED_PARAM(unusedEvent);
        AlertView::showWithMessage("提示", "是否确定退出国标小助手？", 12, []() {
            Director::getInstance()->end();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
            exit(0);
#endif
        }, nullptr);
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

    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText("算番器");
    button->setPosition(Vec2(origin.x + buttonWidth * 0.5f + 4.0f, origin.y + visibleSize.height * 0.5f + buttonHeight * 0.5f + 12.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(FanCalculatorScene::create());
    });

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText("计分器");
    button->setPosition(Vec2(origin.x + buttonWidth * 1.5f + 8.0f, origin.y + visibleSize.height * 0.5f + buttonHeight * 0.5f + 12.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(ScoreSheetScene::create());
    });

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText("番种表");
    button->setPosition(Vec2(origin.x + buttonWidth * 2.5f + 12.0f, origin.y + visibleSize.height * 0.5f + buttonHeight * 0.5f + 12.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(FanTableScene::create());
    });

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText("牌理");
    button->setPosition(Vec2(origin.x + buttonWidth * 0.5f + 4.0f, origin.y + visibleSize.height * 0.5f - buttonHeight * 0.5f + 8.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(MahjongTheoryScene::create());
    });

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText("比赛");
    button->setPosition(Vec2(origin.x + buttonWidth * 1.5f + 8.0f, origin.y + visibleSize.height * 0.5f - buttonHeight * 0.5f + 8.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(CompetitionMainScene::create());
    });

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(buttonWidth, buttonHeight));
    button->setTitleFontSize(20);
    button->setTitleText("其他");
    button->setPosition(Vec2(origin.x + buttonWidth * 2.5f + 12.0f, origin.y + visibleSize.height * 0.5f - buttonHeight * 0.5f + 8.0f));
    button->addClickEventListener([](Ref *) {
        Director::getInstance()->pushScene(OtherScene::create());
    });

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(40.0, 25.0f));
    button->setTitleFontSize(14);
    button->setTitleText("关于");
    button->setPosition(Vec2(origin.x + 23.0f, origin.y + 15.0f));
    button->addClickEventListener(std::bind(&HelloWorld::onAboutButton, this, std::placeholders::_1));

    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(40.0, 25.0f));
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

    requestVersion(false);

    return true;
}

static void shareApplication() {
    const float width = AlertView::maxWidth();

    std::string version = Application::getInstance()->getVersion();
    std::string str = Common::format("<div style=\"word-break:break-all\">https://github.com/summerinsects/ChineseOfficialMahjongHelper/releases/download/v%s/ChineseOfficialMahjongHelper_v%s.apk</div>",
        version.c_str(), version.c_str());

    Node *rootNode = Node::create();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    experimental::ui::WebView *webView = experimental::ui::WebView::create();
    webView->setContentSize(Size(width, 80.0f));
    webView->setBackgroundTransparent();
    webView->setScalesPageToFit(true);
    webView->setOnEnterCallback(std::bind(&experimental::ui::WebView::loadHTMLString, webView, std::ref(str), ""));
    rootNode->addChild(webView);
    webView->setPosition(Vec2(width * 0.5f, 40.0f));
    rootNode->setContentSize(Size(width, 80.0f));
#else
    Label *label = Label::createWithSystemFont(str, "Arail", 10);
    label->setColor(Color3B::BLACK);
    label->setDimensions(width, 0.0f);
    rootNode->addChild(label);

    const Size &labelSize = label->getContentSize();
    rootNode->setContentSize(Size(width, labelSize.height));
    label->setPosition(Vec2(width * 0.5f, labelSize.height * 0.5f));
#endif
    AlertView::showWithNode("下载地址", rootNode, nullptr, nullptr);
}

void HelloWorld::onAboutButton(cocos2d::Ref *) {
    const float width = AlertView::maxWidth();

    Node *rootNode = Node::create();

    Label *label = Label::createWithSystemFont(
        "1. 如果觉得本软件好用，可点击「下载地址」获取下载链接，分享给他人。\n"
        "2. 本软件开源，高端玩家可下载源代码自行编译。\n"
        "3. 由于作者无力承担苹果上架相关费用，没有推出iOS版本，您可以使用源代码自己打包出iOS版本。\n"
        "4. 本项目源代码地址：https://github.com/summerinsects/ChineseOfficialMahjongHelper",
        "Arail", 10);
    label->setColor(Color3B::BLACK);
    label->setDimensions(width, 0.0f);
    rootNode->addChild(label);

    // 检测新版本
    ui::Button *button1 = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button1->setScale9Enabled(true);
    button1->setContentSize(Size(65.0, 20.0f));
    button1->setTitleFontSize(12);
    button1->setTitleText("检测新版本");
    button1->addClickEventListener([this](Ref *) { requestVersion(true); });
    rootNode->addChild(button1);

    ui::Button *button2 = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button2->setScale9Enabled(true);
    button2->setContentSize(Size(65.0, 20.0f));
    button2->setTitleFontSize(12);
    button2->setTitleText("下载地址");
    button2->addClickEventListener([](Ref *) {
        shareApplication();
    });
    rootNode->addChild(button2);

    const Size &labelSize = label->getContentSize();
    rootNode->setContentSize(Size(width, labelSize.height + 30.0f));
    button1->setPosition(Vec2(width * 0.25f, 10.0f));
    button2->setPosition(Vec2(width * 0.75f, 10.0f));
    label->setPosition(Vec2(width * 0.5f, labelSize.height * 0.5f + 30.0f));

    AlertView::showWithNode("关于", rootNode, nullptr, nullptr);
}

void HelloWorld::requestVersion(bool manual) {
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
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
    if (manual) {
        Toast::makeText(this, "获取最新版本失败", Toast::LENGTH_LONG)->show();
    }
#endif
}

static inline bool string_has_suffix(const char *str, const char *suffix) {
    size_t suffix_len = strlen(suffix);
    size_t str_len = strlen(str);
    return (str_len >= suffix_len) && (strcmp(suffix, str + (str_len - suffix_len)) == 0);
}

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

            it = doc.FindMember("assets");
            if (it == doc.MemberEnd() || !it->value.IsArray()) {
                break;
            }

            rapidjson::Value::ConstArray assets = it->value.GetArray();
            if (assets.Size() == 0) {
                break;
            }

            rapidjson::Value::ConstArray::ValueIterator asset = std::find_if(assets.Begin(), assets.End(),
                [](const rapidjson::Value &value) {
                if (!value.IsObject()) return false;
                rapidjson::Value::ConstMemberIterator it = value.FindMember("name");
                if (it == value.MemberEnd() || !it->value.IsString()) return false;
                return string_has_suffix(it->value.GetString(), ".apk");
            });

            if (asset == assets.End()) {
                break;
            }

            it = asset->FindMember("browser_download_url");
            if (it == asset->MemberEnd() || !it->value.IsString()) {
                break;
            }
            std::string url = it->value.GetString();

            it = asset->FindMember("size");
            unsigned size = 0;
            if (it != asset->MemberEnd() && it->value.IsUint()) {
                size = it->value.GetUint();
            }

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

            it = doc.FindMember("body");
            std::string body;
            if (it != doc.MemberEnd() && it->value.IsString()) {
                body = it->value.GetString();
            }

            AlertView::showWithMessage(
                "检测到新版本",
                Common::format("%s，大小%.2fM，是否下载？\n\n%s", tag.c_str(), size / 1048576.0f, body.c_str()), 12, [url]() {
                Application::getInstance()->openURL(url);
            }, nullptr);

            return true;
        } while (0);
    }
    catch (std::exception &e) {
        CCLOG("%s %s", __FUNCTION__, e.what());
    }

    return false;
}
