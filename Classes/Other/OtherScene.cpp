#include "OtherScene.h"

#pragma execution_character_set("utf-8")

USING_NS_CC;

static std::unordered_map<std::string, std::string> g_map;

Scene *OtherScene::createScene() {
    auto scene = Scene::create();
    auto layer = OtherScene::create();
    scene->addChild(layer);
    return scene;
}

bool OtherScene::init() {
    if (!BaseLayer::initWithTitle("其他")) {
        return false;
    }

    if (g_map.empty()) {
        ValueMap valueMap = FileUtils::getInstance()->getValueMapFromFile("other.xml");
        std::for_each(valueMap.begin(), valueMap.end(), [](const ValueMap::value_type &value) {
            g_map.insert(std::make_pair(value.first, value.second.asString()));
        });
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    ui::RichText *richText = ui::RichText::createWithXML("<font face=\"Verdana\" size=\"12\" color=\"#000000\">" + g_map.begin()->second + "</font>");
    richText->setContentSize(Size(visibleSize.width - 10, 0));
    richText->ignoreContentAdaptWithSize(false);
    richText->setVerticalSpace(2);
    richText->formatText();
    this->addChild(richText);
    richText->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height - 40));

    return true;
}
