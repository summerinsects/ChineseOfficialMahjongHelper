#include "OtherScene.h"
#include "../UICommon.h"
#include "../widget/CommonWebViewScene.h"
#include "../widget/LoadingView.h"

USING_NS_CC;

static std::string g_text;

bool OtherScene::init() {
    if (UNLIKELY(!BaseScene::initWithTitle(__UTF8("其他")))) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    ui::Button *button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(90.0f, 32.0f));
    button->setTitleFontSize(20);
    button->setTitleText(__UTF8("相关补充"));
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f + 25.0f));
    button->addClickEventListener(std::bind(&OtherScene::onTipsButton, this, std::placeholders::_1));

    button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(90.0f, 32.0f));
    button->setTitleFontSize(20);
    button->setTitleText(__UTF8("娱乐消遣"));
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 25.0f));
    button->addClickEventListener(std::bind(&OtherScene::onRecreationsButton, this, std::placeholders::_1));

    return true;
}

void OtherScene::onTipsButton(cocos2d::Ref *) {
    if (LIKELY(!g_text.empty())) {
        Director::getInstance()->pushScene(
            CommonWebViewScene::create(__UTF8("相关补充"), g_text, CommonWebViewScene::ContentType::HTML));
        return;
    }

    LoadingView *loadingView = LoadingView::create();
    loadingView->showInScene(this);

    auto thiz = makeRef(this);  // 保证线程回来之前不析构
    std::thread([thiz, loadingView]() {
        // 读文件
        ValueMap valueMap = FileUtils::getInstance()->getValueMapFromFile("text/other.xml");
        auto text = std::make_shared<std::string>();
        if (LIKELY(!valueMap.empty())) {
            *text = valueMap.begin()->second.asString();
        }

        // 切换到cocos线程
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([thiz, loadingView, text]() {
            g_text.swap(*text);
            if (LIKELY(thiz->isRunning())) {
                loadingView->dismiss();
                Director::getInstance()->pushScene(
                    CommonWebViewScene::create(__UTF8("相关补充"), g_text, CommonWebViewScene::ContentType::HTML));
            }
        });
    }).detach();
}

void OtherScene::onRecreationsButton(cocos2d::Ref *) {
    Director::getInstance()->pushScene(
        CommonWebViewScene::create(__UTF8("娱乐消遣"), "http://47.106.11.65/recreations/almanac.html", CommonWebViewScene::ContentType::URL));
}
