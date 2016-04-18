#include "OtherScene.h"

#pragma execution_character_set("utf-8")

USING_NS_CC;

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
    return true;
}
