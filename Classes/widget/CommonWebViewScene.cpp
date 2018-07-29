#include "CommonWebViewScene.h"

USING_NS_CC;

bool CommonWebViewScene::init(const char *title, const std::string &content, CommonWebViewScene::ContentType type) {
    if (UNLIKELY(!BaseScene::initWithTitle(title))) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    const float height = visibleSize.height - 40.0f;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS) && !defined(CC_PLATFORM_OS_TVOS)
    experimental::ui::WebView *webView = experimental::ui::WebView::create();
    webView->setContentSize(Size(visibleSize.width, height));
    webView->setBackgroundTransparent();
    switch (type) {
    case ContentType::URL:
        webView->setOnEnterCallback([webView, content]() { webView->loadURL(content); });
        break;
    case ContentType::HTML:
        webView->setOnEnterCallback([webView, content]() { webView->loadHTMLString(content); });
        break;
    case ContentType::FILE:
        webView->setOnEnterCallback([webView, content]() { webView->loadFile(content); });
        break;
    default:
        break;
    }
    this->addChild(webView);
    webView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + height * 0.5f + 5.0f));
#else
    if (type != ContentType::HTML) {
        if (type == ContentType::URL) {
            Application::getInstance()->openURL(content);
        }
        return true;
    }

    ValueMap defaults;
    defaults.insert(std::make_pair(ui::RichText::KEY_FONT_COLOR_STRING, Value("#000000")));
    ui::RichText *richText = ui::RichText::createWithXML(content, defaults);
    richText->setContentSize(Size(visibleSize.width - 10.0f, 0.0f));
    richText->ignoreContentAdaptWithSize(false);
    richText->setVerticalSpace(2);
    richText->formatText();

    const Size &size = richText->getContentSize();

    // 超出高度就使用ScrollView
    if (size.height <= height) {
        this->addChild(richText);
        richText->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
        richText->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + height - size.height * 0.5f + 5.0f));
    }
    else {
        ui::ScrollView *scrollView = ui::ScrollView::create();
        scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
        scrollView->setScrollBarPositionFromCorner(Vec2(2.0f, 2.0f));
        scrollView->setScrollBarWidth(4.0f);
        scrollView->setScrollBarOpacity(0x99);
        scrollView->setContentSize(Size(size.width, height));
        scrollView->setInnerContainerSize(size);
        scrollView->addChild(richText);
        richText->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
        richText->setPosition(Vec2(size.width * 0.5f, size.height * 0.5f));

        this->addChild(scrollView);
        scrollView->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
        scrollView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + height * 0.5f + 5.0f));
    }
#endif

    return true;
}
