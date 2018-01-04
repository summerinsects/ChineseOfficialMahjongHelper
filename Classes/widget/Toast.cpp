#include "Toast.h"
#include "../utils/compiler.h"

USING_NS_CC;

Toast *Toast::makeText(cocos2d::Scene *scene, const std::string &text, Duration duration) {
    Toast *ret = new (std::nothrow) Toast();
    if (ret != nullptr && ret->initWithText(scene, text, duration)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool Toast::initWithText(cocos2d::Scene *scene, const std::string &text, Duration duration) {
    if (UNLIKELY(!Node::init())) {
        return false;
    }

    _scene = scene;
    _duration = duration;

    const float width = maxWidth();

    Label *label = Label::createWithSystemFont(text, "Arail", 12);
    Size size = label->getContentSize();
    if (size.width > width - 15.0f) {
        label->setHorizontalAlignment(TextHAlignment::CENTER);
        label->setDimensions(width - 15.0f, 0.0f);
        size = label->getContentSize();
    }

    size.width += 15.0f;
    size.height += 15.0f;
    this->setContentSize(Size(size.width, size.height));

    this->addChild(label);
    label->setPosition(Vec2(size.width * 0.5f, size.height * 0.5f));

    this->addChild(LayerColor::create(Color4B(0x20, 0x20, 0x20, 0xF0), size.width, size.height), -1);

    this->setIgnoreAnchorPointForPosition(false);
    this->setAnchorPoint(Vec2::ANCHOR_MIDDLE);

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    float yPos = visibleSize.height * 0.15f;
    if (yPos + size.height * 0.5f > visibleSize.height) {
        yPos = visibleSize.height * 0.5f;
    }
    this->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + yPos));

    return true;
}

void Toast::show() {
    _scene->addChild(this, 100);
    _scene = nullptr;

    float dt = _duration == LENGTH_LONG ? 3.5f : 2.0f;
    this->runAction(Sequence::create(DelayTime::create(dt), FadeOut::create(0.2f), RemoveSelf::create(), NULL));
}
