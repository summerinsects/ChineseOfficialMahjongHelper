#include "ScoreDefinition.h"
#include "../common.h"
#include "../mahjong-algorithm/points_calculator.h"

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
                    "<img src=\"%s\" width=\"%d\" height=\"%d\"/>", tilesImageName[tiles[i]],
                    (int)(27 * scale), (int)(39 * scale));
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
    if (g_vec.empty()) {
        ValueVector valueVec = FileUtils::getInstance()->getValueVectorFromFile("score_definition.xml");
        g_vec.reserve(valueVec.size());
        std::transform(valueVec.begin(), valueVec.end(), std::back_inserter(g_vec), std::bind(&Value::asString, std::placeholders::_1));
    }

    auto scene = Scene::create();
    auto layer = new (std::nothrow) ScoreDefinitionScene();
    layer->initWithIndex(idx);

    scene->addChild(layer);
    return scene;
}

bool ScoreDefinitionScene::initWithIndex(size_t idx) {
    if (!BaseLayer::initWithTitle(mahjong::points_name[idx])) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

#if HAS_CONCEALED_KONG_AND_MELDED_KONG
    if (idx > mahjong::POINT_TYPE::CONCEALED_KONG_AND_MELDED_KONG) {
        --idx;
    }
#endif

    std::string &text = g_vec[idx];
    float scale = 1.0f;
    float maxWidth = (visibleSize.width - 10) / 18;
    if (maxWidth < 25) {
        scale = maxWidth / 27;
    }
    replaceTilesToImage(text, scale);

    ui::RichText *richText = ui::RichText::createWithXML(text);
    richText->setContentSize(Size(visibleSize.width - 10, 0));
    richText->ignoreContentAdaptWithSize(false);
    richText->setVerticalSpace(2);
    richText->formatText();
    this->addChild(richText);
    richText->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 40));
    return true;
}
