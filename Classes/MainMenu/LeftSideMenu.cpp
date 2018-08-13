#include "LeftSideMenu.h"
#include "network/HttpClient.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "../utils/compiler.h"
#include "../utils/common.h"
#include "../UICommon.h"
#include "../UIColors.h"
#include "../widget/AlertDialog.h"
#include "../widget/Toast.h"
#include "../widget/LoadingView.h"
#include "../widget/CommonWebViewScene.h"

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

bool LeftSideMenu::init(cocos2d::Scene *scene) {
    if (UNLIKELY(!Layer::init())) {
        return false;
    }

    _scene = scene;

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

    if (SpriteFrameCache::getInstance()->getSpriteFrameByName("popmenu_item_pressed") == nullptr) {
        // 3平方像素图片编码
        Sprite *sprite = utils::createSpriteFromBase64("iVBORw0KGgoAAAANSUhEUgAAAAMAAAADCAYAAABWKLW/AAAAEklEQVR42mPYu3fvWRhmwMkBABCrGyXDqPNeAAAAAElFTkSuQmCC");
        SpriteFrameCache::getInstance()->addSpriteFrame(sprite->getSpriteFrame(), "popmenu_item_pressed");
    }

    static const std::pair<const char *, void (LeftSideMenu::*)(Ref *)> menuItems[] = {
        { __UTF8("设置"), &LeftSideMenu::onSettingButton },
        { __UTF8("分享下载"), &LeftSideMenu::onSharedButton },
        { __UTF8("捐赠"), &LeftSideMenu::onDonationButton },
        { __UTF8("更新日志"), &LeftSideMenu::onUpdateLogButton },
        { __UTF8("版本检测"), &LeftSideMenu::onVersionCheckButton },
        { __UTF8("退出"), &LeftSideMenu::onExitButton },
    };

    Label *label = Label::createWithSystemFont(__UTF8("国标小助手"), "Arial", 14);
    label->setTextColor(C4B_BLACK);
    background->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_BOTTOM);
    label->setPosition(Vec2(maxWidth * 0.5f, visibleSize.height - maxWidth * 0.5f + 5.0f));

    std::string version = Application::getInstance()->getVersion();
    label = Label::createWithSystemFont(version.insert(0, 1, 'v').append("\n" __DATE__ "  " __TIME__), "Arial", 10);
    label->setTextColor(C4B_GRAY);
    background->addChild(label);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_TOP);
    label->setAlignment(TextHAlignment::CENTER);
    label->setPosition(Vec2(maxWidth * 0.5f, visibleSize.height - maxWidth * 0.5f));
    cw::scaleLabelToFitWidth(label, maxWidth - 4.0f);

    const float yPosTop = visibleSize.height - maxWidth;  // 预留一个正方型区域
    for (size_t i = 0, cnt = _countof(menuItems); i < cnt; ++i) {
        ui::Button *button = ui::Button::create();
        button->loadTexturePressed("popmenu_item_pressed", ui::Widget::TextureResType::PLIST);
        background->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(maxWidth, 40.0f));
        button->setTitleFontSize(14);
        button->setTitleColor(C3B_GRAY);
        button->setTitleText(menuItems[i].first);
        button->setPosition(Vec2(maxWidth * 0.5f, yPosTop - (i + 0.5f) * 40.0f));
        button->addClickEventListener(std::bind(menuItems[i].second, this, std::placeholders::_1));
    }

    // 分隔线
    DrawNode *drawNode = DrawNode::create();
    background->addChild(drawNode);
    for (size_t i = 0, cnt = _countof(menuItems) + 1; i < cnt; ++i) {
        const float yPos = yPosTop - i * 40.0f;
        drawNode->drawLine(Vec2(0.0f, yPos), Vec2(maxWidth, yPos), Color4F::GRAY);
    }

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

void LeftSideMenu::onSettingButton(cocos2d::Ref *) {

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

void LeftSideMenu::onSharedButton(cocos2d::Ref *) {
    Texture2D *texture = Director::getInstance()->getTextureCache()->getTextureForKey("shared_qrcode_texture");
    if (texture != nullptr) {
        showQRCodeAlertDialog(_scene, texture);
        return;
    }

    static bool isSending = false;
    if (isSending) {
        return;
    }
    isSending = true;

    const float realWidth = AlertDialog::maxWidth() * CC_CONTENT_SCALE_FACTOR();
    const int factor = static_cast<int>(realWidth / 43.0f);

    network::HttpRequest *request = new (std::nothrow) network::HttpRequest();
    request->setRequestType(network::HttpRequest::Type::GET);
    request->setUrl(Common::format("%s?pixsize=%d", QR_CODE_URL, factor * 43));

    LoadingView *loadingView = LoadingView::create();
    loadingView->showInScene(_scene);
    auto scene = makeRef(_scene);
    request->setResponseCallback([scene, loadingView](network::HttpClient *client, network::HttpResponse *response) {
        CC_UNUSED_PARAM(client);
        network::HttpClient::destroyInstance();

        isSending = false;

        loadingView->dismiss();
        if (response == nullptr) {
            return;
        }

        log("HTTP Status Code: %ld", response->getResponseCode());

        if (!response->isSucceed()) {
            log("response failed");
            log("error buffer: %s", response->getErrorBuffer());
            Toast::makeText(scene.get(), __UTF8("获取二维码失败"), Toast::LENGTH_LONG)->show();
            return;
        }

        std::vector<char> *buffer = response->getResponseData();
        if (buffer == nullptr) {
            Toast::makeText(scene.get(), __UTF8("获取二维码失败"), Toast::LENGTH_LONG)->show();
            return;
        }

        Image *image = new (std::nothrow) Image();
        image->initWithImageData(reinterpret_cast<const unsigned char *>(buffer->data()), static_cast<ssize_t>(buffer->size()));
        Texture2D *texture = Director::getInstance()->getTextureCache()->addImage(image, "shared_qrcode_texture");
        image->release();
        showQRCodeAlertDialog(scene.get(), texture);
    });

    network::HttpClient::getInstance()->send(request);
    request->release();
}

void LeftSideMenu::onDonationButton(cocos2d::Ref *) {
    Application::getInstance()->openURL("https://gitee.com/summerinsects/ChineseOfficialMahjongHelper?donate=true&&skip_mobile=true");
}

static void showChangeLog(Scene *scene, const std::string &str) {
    Director::getInstance()->pushScene(
        CommonWebViewScene::create(__UTF8("更新日志"), str, CommonWebViewScene::ContentType::HTML));
}

void LeftSideMenu::onUpdateLogButton(cocos2d::Ref *) {
    static std::string changelog;
    if (!changelog.empty()) {
        showChangeLog(_scene, changelog);
        return;
    }

    static bool isSending = false;
    if (isSending) {
        return;
    }
    isSending = true;

    network::HttpRequest *request = new (std::nothrow) network::HttpRequest();
    request->setRequestType(network::HttpRequest::Type::GET);
    request->setUrl("https://raw.githubusercontent.com/summerinsects/ChineseOfficialMahjongHelper/master/CHANGELOG");

    LoadingView *loadingView = LoadingView::create();
    loadingView->showInScene(_scene);
    auto scene = makeRef(_scene);
    request->setResponseCallback([scene, loadingView](network::HttpClient *client, network::HttpResponse *response) {
        CC_UNUSED_PARAM(client);
        network::HttpClient::destroyInstance();

        isSending = false;

        loadingView->dismiss();
        if (response == nullptr) {
            return;
        }

        log("HTTP Status Code: %ld", response->getResponseCode());

        if (!response->isSucceed()) {
            log("response failed");
            log("error buffer: %s", response->getErrorBuffer());
            Toast::makeText(scene.get(), __UTF8("获取更新日志失败"), Toast::LENGTH_LONG)->show();
            return;
        }

        std::vector<char> *buffer = response->getResponseData();
        if (buffer == nullptr) {
            Toast::makeText(scene.get(), __UTF8("获取更新日志失败"), Toast::LENGTH_LONG)->show();
            return;
        }

        std::string temp;

        temp.reserve(buffer->size() + 4 * std::count(buffer->cbegin(), buffer->cend(), '\n'));
        std::vector<char>::const_iterator it1, it2;
        it1 = buffer->cbegin();
        do {
            it2 = std::find(it1, buffer->cend(), '\n');
            temp.append(it1, it2);

            if (it2 == buffer->cend()) break;

            temp.append("<br/>");
            it1 = it2 + 1;
        } while (1);

        changelog.swap(temp);
        showChangeLog(scene.get(), changelog);
    });

    network::HttpClient::getInstance()->send(request);
    request->release();
}

void LeftSideMenu::onVersionCheckButton(cocos2d::Ref *) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)

    static bool isSending = false;
    if (isSending) {
        return;
    }
    isSending = true;

    network::HttpRequest *request = new (std::nothrow) network::HttpRequest();
    request->setRequestType(network::HttpRequest::Type::GET);
    request->setUrl("https://api.github.com/repos/summerinsects/ChineseOfficialMahjongHelper/releases/latest");

    request->setResponseCallback([this](network::HttpClient *client, network::HttpResponse *response) {
        CC_UNUSED_PARAM(client);
        network::HttpClient::destroyInstance();

        isSending = false;

        if (response == nullptr) {
            return;
        }

        log("HTTP Status Code: %ld", response->getResponseCode());

        if (!response->isSucceed()) {
            log("response failed");
            log("error buffer: %s", response->getErrorBuffer());
            Toast::makeText(_scene, __UTF8("获取最新版本失败"), Toast::LENGTH_LONG)->show();
            return;
        }

        std::vector<char> *buffer = response->getResponseData();
        if (!checkVersion(buffer)) {
            Toast::makeText(_scene, __UTF8("获取最新版本失败"), Toast::LENGTH_LONG)->show();
        }
    });

    network::HttpClient::getInstance()->send(request);
    request->release();
#else
    Toast::makeText(_scene, __UTF8("当前平台不支持该操作"), Toast::LENGTH_LONG)->show();
#endif
}

void LeftSideMenu::onExitButton(cocos2d::Ref *) {
    AlertDialog::Builder(_scene)
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
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
bool LeftSideMenu::checkVersion(const std::vector<char> *buffer) {
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
        bool hasNewVersion = Common::compareVersion(tag.c_str() + 1, Application::getInstance()->getVersion().c_str());

        UserDefault *userDefault = UserDefault::getInstance();
        userDefault->setBoolForKey("has_new_version", hasNewVersion);
        userDefault->setStringForKey("last_request_time", std::to_string(time(nullptr)));
        userDefault->setBoolForKey("notify_tomorrow", false);

        if (!hasNewVersion) {
            Toast::makeText(_scene, __UTF8("已经是最新版本"), Toast::LENGTH_LONG)->show();
            return true;
        }

        it = doc.FindMember("body");
        std::string body;
        if (it != doc.MemberEnd() && it->value.IsString()) {
            body = it->value.GetString();
        }

        AlertDialog::Builder(_scene)
            .setTitle(__UTF8("检测到新版本"))
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
            .setMessage(Common::format(__UTF8("%s，是否下载？\n\n%s"), tag.c_str(), body.c_str()))
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
            .setMessage(Common::format(__UTF8("%s，是否下载？\n提取密码xyg\n\n%s"), tag.c_str(), body.c_str()))
#endif
            .setCloseOnTouchOutside(false)
            .setNegativeButton(__UTF8("取消"), nullptr)
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
