#include "ScoreDefinition.h"
#include "ui/UIWebView.h"
#include "../common.h"
#include "../mahjong-algorithm/points_calculator.h"
#include "../compiler.h"
#include <thread>

USING_NS_CC;

static std::vector<std::string> g_vec;

static void replaceTilesToImage(std::string &text, float scale) {
    char tilesStr[128];
    mahjong::TILE tiles[14];
    long tilesCnt;
    char imgStr[1024];

    std::string::size_type pos = text.find('[');
    while (pos != std::string::npos) {
        const char *str = text.c_str();
        int readLen;
        if (sscanf(str + pos + 1, "%[^]]%n", tilesStr, &readLen) != EOF
            && str[pos + readLen + 1] == ']'
            && mahjong::parse_tiles(tilesStr, tiles, &tilesCnt) != nullptr) {
            size_t totalWriteLen = 0;
            for (long i = 0; i < tilesCnt; ++i) {
                int writeLen = snprintf(imgStr + totalWriteLen, sizeof(imgStr) - totalWriteLen,
                    "<img src=\"%s\" width=\"%d\" height=\"%d\"/>",
                    tilesImageName[tiles[i]], (int)(27 * scale), (int)(39 * scale));
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

Scene *ScoreDefinitionScene::createScene(size_t idx) {
    auto scene = Scene::create();
    auto layer = new (std::nothrow) ScoreDefinitionScene();
    layer->initWithIndex(idx);
    layer->autorelease();

    scene->addChild(layer);
    return scene;
}

bool ScoreDefinitionScene::initWithIndex(size_t idx) {
    if (!BaseLayer::initWithTitle(mahjong::points_name[idx])) {
        return false;
    }

    if (LIKELY(!g_vec.empty())) {
        createContentView(idx);
    }
    else {
        Size visibleSize = Director::getInstance()->getVisibleSize();
        Vec2 origin = Director::getInstance()->getVisibleOrigin();

        Sprite *sprite = Sprite::create("source_material/loading_black.png");
        this->addChild(sprite);
        sprite->setScale(40 / sprite->getContentSize().width);
        sprite->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 15.0f));
        sprite->runAction(RepeatForever::create(RotateBy::create(0.5f, 180.0f)));

        float scale = 1.0f;
        float maxWidth = (visibleSize.width - 10) / 18;
        if (maxWidth < 25) {
            scale = maxWidth / 27;
        }

        auto thiz = RefPtr<ScoreDefinitionScene>(this);
        std::thread thread([thiz, idx, scale, sprite]() {
            ValueVector valueVec = FileUtils::getInstance()->getValueVectorFromFile("score_definition.xml");
            g_vec.reserve(valueVec.size());
            std::transform(valueVec.begin(), valueVec.end(), std::back_inserter(g_vec),
                std::bind(&Value::asString, std::placeholders::_1));
            std::for_each(g_vec.begin(), g_vec.end(),
                std::bind(&replaceTilesToImage, std::placeholders::_1, scale));

            Director::getInstance()->getScheduler()->performFunctionInCocosThread([thiz, idx, sprite]() {
                if (LIKELY(thiz->getParent() != nullptr)) {
                    sprite->removeFromParent();
                    thiz->createContentView(idx);
                }
            });
        });
        thread.detach();
    }

    return true;
}

void ScoreDefinitionScene::createContentView(size_t idx) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

#if HAS_CONCEALED_KONG_AND_MELDED_KONG
    if (idx > mahjong::POINT_TYPE::CONCEALED_KONG_AND_MELDED_KONG) {
        --idx;
    }
#endif

    const std::string &text = g_vec[idx];

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS) && !defined(CC_PLATFORM_OS_TVOS)
    experimental::ui::WebView *webView = experimental::ui::WebView::create();
    webView->setContentSize(Size(visibleSize.width, visibleSize.height - 35));
    webView->loadHTMLString(text, "");
    this->addChild(webView);
    webView->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f - 15.0f));
#else
    ui::RichText *richText = ui::RichText::createWithXML(text);
    richText->setContentSize(Size(visibleSize.width - 10, 0));
    richText->ignoreContentAdaptWithSize(false);
    richText->setVerticalSpace(2);
    richText->formatText();
    this->addChild(richText);
    richText->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 40.0f));
#endif
}
