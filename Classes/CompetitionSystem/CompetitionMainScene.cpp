#include "CompetitionMainScene.h"
#include "../UICommon.h"
#include "LatestCompetitionScene.h"

USING_NS_CC;

bool CompetitionMainScene::init() {
    if (UNLIKELY(!BaseScene::initWithTitle(__UTF8("比赛")))) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    ui::Button *button = UICommon::createButton();
    this->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(90.0f, 32.0f));
    button->setTitleFontSize(20);
    button->setTitleText(__UTF8("近期赛事"));
    button->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f));
    button->addClickEventListener([this](Ref *) {
        Director::getInstance()->pushScene(LatestCompetitionScene::create());
    });

    return true;
}
