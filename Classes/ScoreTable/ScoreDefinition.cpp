#include "ScoreDefinition.h"
#include "../common.h"
#include "../mahjong-algorithm/points_calculator.h"

USING_NS_CC;

static std::vector<std::string> g_vec;

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
    if (!Layer::init()) {
        return false;
    }

    auto listener = EventListenerKeyboard::create();
    listener->onKeyReleased = CC_CALLBACK_2(Layer::onKeyReleased, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    Label *tileLabel = Label::createWithSystemFont(mahjong::points_name[idx], "Arial", 24);
    this->addChild(tileLabel);
    tileLabel->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height - tileLabel->getContentSize().height * 0.5f));

    ui::Button *backBtn = ui::Button::create();
    this->addChild(backBtn);
    backBtn->setScale9Enabled(true);
    backBtn->setContentSize(Size(45, 20));
    backBtn->setTitleFontSize(16);
    backBtn->setTitleText("返回");
    backBtn->setPosition(Vec2(origin.x + 15, origin.y + visibleSize.height - 10));
    backBtn->addClickEventListener([](Ref *) {
        Director::getInstance()->popScene();
    });

    std::string &text = g_vec[idx];
    float scale = 1.0f;
    float maxWidth = (visibleSize.width - 10) / 18;
    if (maxWidth < 25) {
        scale = maxWidth / 27;
    }

    char tilesStr[128];
    mahjong::TILE tiles[14];
    long tilesCnt;
    char imgStr[1024];

    std::string::size_type pos = text.find('[');
    while (pos != std::string::npos) {
        sscanf(text.c_str() + pos + 1, "%[^]]", tilesStr);
        if (mahjong::parse_tiles(tilesStr, tiles, &tilesCnt) != nullptr) {
            char *p = imgStr;
            for (long i = 0; i < tilesCnt; ++i) {
                int n = snprintf(p, sizeof(imgStr) - (p - imgStr), "<img src=\"%s\" width=\"%d\" height=\"%d\"/>", tilesImageName[tiles[i]], (int)(27 * scale), (int)(39 * scale));
                p += n;
            }
            text.replace(pos, text.find(']', pos) - pos + 1, imgStr);
            pos = text.find('[', pos + (p - imgStr));
        }
        else {
            pos = text.find('[', pos + 1);
        }
    }

    ui::RichText *richText = ui::RichText::createWithXML(text);
    richText->setContentSize(Size(visibleSize.width - 10, 0));
    richText->ignoreContentAdaptWithSize(false);
    richText->setVerticalSpace(2);
    richText->formatText();
    this->addChild(richText);
    richText->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 40));
    return true;
}

void ScoreDefinitionScene::onKeyReleased(EventKeyboard::KeyCode keyCode, Event *unusedEvent) {
    if (keyCode == EventKeyboard::KeyCode::KEY_BACK) {
        Director::getInstance()->popScene();
    }
}
