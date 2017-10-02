#include "HelloWorldScene.h"
#include "network/HttpClient.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "utils/common.h"
#include "widget/AlertView.h"
#include "FanCalculator/FanCalculatorScene.h"
#include "RecordSystem/ScoreSheetScene.h"
#include "FanTable/FanTableScene.h"
#include "Other/OtherScene.h"
#include "MahjongTheory/MahjongTheoryScene.h"
#include "LatestCompetition/LatestCompetitionScene.h"
#include "CompetitionSystem/CompetitionMainScene.h"

#define VERSION 0x010108

USING_NS_CC;

static bool checkVersion(const std::vector<char> *buffer, bool manual);

bool HelloWorld::init() {
    if (UNLIKELY(!Scene::init())) {
        return false;
    }

    UserDefault::getInstance()->deleteValueForKey("night_mode");

    LayerColor *background = LayerColor::create(COLOR4B_BG);
    this->addChild(background, -100);

    auto listener = EventListenerKeyboard::create();
    listener->onKeyReleased = [](EventKeyboard::KeyCode keyCode, Event *unused_event) {
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
    button->setTitleText("近期赛事");
    button->setPosition(Vec2(origin.x + buttonWidth * 1.5f + 8.0f, origin.y + visibleSize.height * 0.5f - buttonHeight * 0.5f + 8.0f));
    button->addClickEventListener([](Ref *) {
        //Director::getInstance()->pushScene(LatestCompetitionScene::create());
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
        Application::getInstance()->openURL("https://git.oschina.net/201103L/ChineseOfficialMahjongHelper?donate=true&&skip_mobile=true");
    });

    Label *label = Label::createWithSystemFont(
        Common::format("v%d.%d.%d\n%s", (VERSION >> 16) & 0xFF, (VERSION >> 8) & 0xFF, VERSION & 0xFF, "Built  " __DATE__ "  " __TIME__),
        "Arial", 10);
    label->setColor(Color3B::BLACK);
    this->addChild(label);
    label->setAlignment(TextHAlignment::CENTER);
    label->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + 15.0f));

    requestVersion(false);

    return true;
}

void HelloWorld::onAboutButton(cocos2d::Ref *sender) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float width = visibleSize.width * 0.8f - 10.0f;

    Node *rootNode = Node::create();

    Label *label = Label::createWithSystemFont(
        "1. 本软件开源，高端玩家可下载源代码自行编译。\n"
        "2. 由于作者无力承担苹果上架相关费用，没有推出iOS版本，您可以使用源代码自己打包出iOS版本。\n"
        "3. 本项目源代码地址：https://github.com/summerinsects/ChineseOfficialMahjongHelper",
        "Arail", 10);
    label->setColor(Color3B::BLACK);
    label->setDimensions(width, 0.0f);
    rootNode->addChild(label);

    // 检测新版本
    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(65.0, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("检测新版本");
    button->addClickEventListener([this](Ref *sender) { requestVersion(true); });
    rootNode->addChild(button);

    const Size &labelSize = label->getContentSize();
    rootNode->setContentSize(Size(width, labelSize.height + 30.0f));
    button->setPosition(Vec2(width * 0.5f, 10.0f));
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

    request->setResponseCallback([manual](network::HttpClient *client, network::HttpResponse *response) {
        checking = false;

        if (response == nullptr) {
            return;
        }

        log("HTTP Status Code: %ld", response->getResponseCode());

        if (!response->isSucceed()) {
            log("response failed");
            log("error buffer: %s", response->getErrorBuffer());
            if (manual) {
                AlertView::showWithMessage("提示", "获取最新版本失败", 12, nullptr, nullptr);
            }
            return;
        }

        std::vector<char> *buffer = response->getResponseData();
        if (!checkVersion(buffer, manual)) {
            if (manual) {
                AlertView::showWithMessage("提示", "获取最新版本失败", 12, nullptr, nullptr);
            }
        }
    });

    network::HttpClient::getInstance()->send(request);
    request->release();
#else
    if (manual) {
        AlertView::showWithMessage("提示", "获取最新版本失败", 12, nullptr, nullptr);
    }
#endif
}

static inline bool string_has_suffix(const char *str, const char *suffix) {
    size_t suffix_len = strlen(suffix);
    size_t str_len = strlen(str);
    return (str_len >= suffix_len) && (strcmp(suffix, str + (str_len - suffix_len)) == 0);
}

bool checkVersion(const std::vector<char> *buffer, bool manual) {
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

            int a, b, c;
            if (sscanf(tag.c_str(), "v%d.%d.%d", &a, &b, &c) != 3) {
                break;
            }

            if (((a << 16) | (b << 8) | c) <= VERSION) {
                if (manual) {
                    AlertView::showWithMessage("提示", "已经是最新版本", 12, nullptr, nullptr);
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
                Common::format("%s，大小%.2fM，是否下载？\n%s", tag.c_str(), size / 1048576.0f, body.c_str()), 12, [url]() {
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
