#include "FanDefinitionScene.h"
#include <algorithm>
#include <iterator>
#include "../mahjong-algorithm/stringify.h"
#include "../mahjong-algorithm/fan_calculator.h"
#include "../TilesImage.h"
#include "../widget/LoadingView.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS) && !defined(CC_PLATFORM_OS_TVOS)
#define HAS_WEBVIEW 1
#else
#define HAS_WEBVIEW 0
#endif

USING_NS_CC;

static const char *principle_title[] = { "不重复原则", "不拆移原则", "不得相同原则", "就高不就低原则", "套算一次原则" };

static std::vector<std::string> g_principles;
static std::vector<std::string> g_definitions;

static void replaceTilesToImage(std::string &text, float scale) {
    const int width = static_cast<int>(TILE_WIDTH * scale);
    const int height = static_cast<int>(TILE_HEIGHT * scale);

    char tilesStr[128];
    mahjong::tile_t tiles[14];
    char imgStr[1024];

    std::string::size_type pos = text.find('[');
    while (pos != std::string::npos) {
        const char *str = text.c_str();
        int readLen;
        if (sscanf(str + pos + 1, "%[^]]%n", tilesStr, &readLen) != EOF
            && str[pos + readLen + 1] == ']') {

            size_t totalWriteLen = 0;
            const char *p = tilesStr;
            if (*p == '_') {
                int writeLen = snprintf(imgStr, sizeof(imgStr),
                    "<img src=\"tiles/bg.png\" width=\"%d\" height=\"%d\"/>", width, height);
                totalWriteLen += writeLen;
                ++p;
            }

            intptr_t tilesCnt = mahjong::parse_tiles(p, tiles, 14);
            for (intptr_t i = 0; i < tilesCnt; ++i) {
                int writeLen = snprintf(imgStr + totalWriteLen, sizeof(imgStr) - totalWriteLen,
                    "<img src=\"%s\" width=\"%d\" height=\"%d\"/>", tilesImageName[tiles[i]], width, height);
                totalWriteLen += writeLen;
            }

            if (tilesStr[readLen - 1] == '_') {
                int writeLen = snprintf(imgStr + totalWriteLen, sizeof(imgStr) - totalWriteLen,
                    "<img src=\"tiles/bg.png\" width=\"%d\" height=\"%d\"/>", width, height);
                totalWriteLen += writeLen;
            }

            text.replace(pos, readLen + 2, imgStr);
            pos = text.find('[', pos + totalWriteLen);
        }
        else {
            pos = text.find('[', pos + 1);
        }
    }
}

bool FanDefinitionScene::initWithIndex(size_t idx) {
    const char *title = idx < 100 ? mahjong::fan_name[idx] : principle_title[idx - 100];
    if (UNLIKELY(!BaseScene::initWithTitle(title))) {
        return false;
    }

    if (LIKELY(g_definitions.size() == 82 && g_principles.size() == 5)) {
        createContentView(idx);
        return true;
    }

    this->scheduleOnce([this, idx](float) {
        Vec2 origin = Director::getInstance()->getVisibleOrigin();

        LoadingView *loadingView = LoadingView::create();
        this->addChild(loadingView);
        loadingView->setPosition(origin);

#if HAS_WEBVIEW
        Size visibleSize = Director::getInstance()->getVisibleSize();

        float scale = 1.0f;
        float maxWidth = (visibleSize.width - 10) / 18;
        if (maxWidth < 25) {
            scale = maxWidth / TILE_WIDTH;
        }
#else
        float scale = 0.5f;
#endif

        auto thiz = makeRef(this);  // 保证线程回来之前不析构
        std::thread([thiz, idx, scale, loadingView]() {
            // 读文件
            auto definitions = std::make_shared<std::vector<std::string> >();
            ValueVector valueVec = FileUtils::getInstance()->getValueVectorFromFile("text/score_definition.xml");
            if (valueVec.size() == 82) {
                definitions->reserve(82);
                std::transform(valueVec.begin(), valueVec.end(), std::back_inserter(*definitions), [scale](const Value &value) {
                    std::string ret = value.asString();
                    replaceTilesToImage(ret, scale);
                    return std::move(ret);
                });
            }

            auto principles = std::make_shared<std::vector<std::string> >();
            valueVec = FileUtils::getInstance()->getValueVectorFromFile("text/score_principles.xml");
            if (valueVec.size() == 5) {
                principles->reserve(5);
                std::transform(valueVec.begin(), valueVec.end(), std::back_inserter(*principles), [scale](const Value &value) {
                    std::string ret = value.asString();
                    replaceTilesToImage(ret, scale);
                    return std::move(ret);
                });
            }

            // 切换到cocos线程
            Director::getInstance()->getScheduler()->performFunctionInCocosThread([thiz, idx, loadingView, definitions, principles]() {
                g_definitions.swap(*definitions);
                g_principles.swap(*principles);

                if (LIKELY(thiz->isRunning())) {
                    loadingView->removeFromParent();
                    thiz->createContentView(idx);
                }
            });
        }).detach();
    }, 0.0f, "load_texts");

    return true;
}

void FanDefinitionScene::createContentView(size_t idx) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    const float height = visibleSize.height - 40.0f;

    const std::string &text = idx < 100 ? g_definitions[idx] : g_principles[idx - 100];

#if HAS_WEBVIEW
    experimental::ui::WebView *webView = experimental::ui::WebView::create();
    webView->setContentSize(Size(visibleSize.width, height));
    webView->setBackgroundTransparent();
    webView->setOnEnterCallback(std::bind(&experimental::ui::WebView::loadHTMLString, webView, std::ref(text), ""));
    this->addChild(webView);
    webView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + height * 0.5f + 5.0f));
#else
    ui::RichText *richText = ui::RichText::createWithXML("<font face=\"Verdana\" size=\"12\" color=\"#000000\">" + text + "</font>");
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
        scrollView->setBounceEnabled(true);
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
}
