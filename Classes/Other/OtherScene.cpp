#include "OtherScene.h"
#include "../widget/LoadingView.h"

USING_NS_CC;

static std::string g_text;

bool OtherScene::init() {
    if (UNLIKELY(!BaseScene::initWithTitle("其他"))) {
        return false;
    }

    if (LIKELY(!g_text.empty())) {
        createContentView();
    }
    else {
        Vec2 origin = Director::getInstance()->getVisibleOrigin();

        LoadingView *loadingView = LoadingView::create();
        this->addChild(loadingView);
        loadingView->setPosition(origin);

        auto thiz = makeRef(this);  // 保证线程回来之前不析构
        std::thread([thiz, loadingView]() {
            // 读文件
            ValueMap valueMap = FileUtils::getInstance()->getValueMapFromFile("text/other.xml");
            if (LIKELY(!valueMap.empty())) {
                g_text = valueMap.begin()->second.asString();
            }

            // 切换到cocos线程
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([thiz, loadingView]() {
                if (LIKELY(thiz->isRunning())) {
                    loadingView->removeFromParent();
                    thiz->createContentView();
                }
            });
        }).detach();
    }

    return true;
}

void OtherScene::createContentView() {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS) && !defined(CC_PLATFORM_OS_TVOS)
    experimental::ui::WebView *webView = experimental::ui::WebView::create();
    webView->setContentSize(Size(visibleSize.width, visibleSize.height - 35));
    webView->setOnEnterCallback(std::bind(&experimental::ui::WebView::loadHTMLString, webView, std::ref(g_text), ""));
    this->addChild(webView);
    webView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 15.0f));
#else
    ui::RichText *richText = ui::RichText::createWithXML("<font face=\"Verdana\" size=\"12\" color=\"#000000\">" + g_text + "</font>");
    richText->setContentSize(Size(visibleSize.width - 10, 0));
    richText->ignoreContentAdaptWithSize(false);
    richText->setVerticalSpace(2);
    richText->formatText();
    this->addChild(richText);
    richText->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 40.0f));
#endif
}
