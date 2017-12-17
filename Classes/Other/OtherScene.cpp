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
        return true;
    }

    this->scheduleOnce([this](float) {
        Vec2 origin = Director::getInstance()->getVisibleOrigin();

        LoadingView *loadingView = LoadingView::create();
        this->addChild(loadingView);
        loadingView->setPosition(origin);

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
                    loadingView->removeFromParent();
                    thiz->createContentView();
                }
            });
        }).detach();
    }, 0.0f, "load_texts");

    return true;
}

void OtherScene::createContentView() {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    const float height = visibleSize.height - 40.0f;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS) && !defined(CC_PLATFORM_OS_TVOS)
    experimental::ui::WebView *webView = experimental::ui::WebView::create();
    webView->setContentSize(Size(visibleSize.width, height));
    webView->setBackgroundTransparent();
    webView->setOnEnterCallback(std::bind(&experimental::ui::WebView::loadHTMLString, webView, std::ref(g_text), ""));
    this->addChild(webView);
    webView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + height * 0.5f + 0.5f));
#else
    ui::RichText *richText = ui::RichText::createWithXML("<font face=\"Verdana\" size=\"12\" color=\"#000000\">" + g_text + "</font>");
    richText->setContentSize(Size(visibleSize.width - 10.0f, 0.0f));
    richText->ignoreContentAdaptWithSize(false);
    richText->setVerticalSpace(2);
    richText->formatText();

    const Size &size = richText->getContentSize();

    // 超出高度就使用ScrollView
    if (size.height <= height) {
        this->addChild(richText);
        richText->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
        richText->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + height - size.height * 0.5f + 0.5f));
    }
    else {
        ui::ScrollView *scrollView = ui::ScrollView::create();
        scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
        scrollView->setScrollBarPositionFromCorner(Vec2(2.0f, 2.0f));
        scrollView->setScrollBarWidth(4.0f);
        scrollView->setScrollBarOpacity(0x99);
        scrollView->setBounceEnabled(true);
        scrollView->setContentSize(Size(size.width, height));
        scrollView->setInnerContainerSize(size);
        scrollView->addChild(richText);
        richText->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
        richText->setPosition(Vec2(size.width * 0.5f, size.height * 0.5f));

        this->addChild(scrollView);
        scrollView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
        scrollView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + height * 0.5f + 0.5f));
    }
#endif
}
