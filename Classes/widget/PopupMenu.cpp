#include "PopupMenu.h"
#include "../utils/compiler.h"
#include "../UIColors.h"

USING_NS_CC;

bool PopupMenu::init(cocos2d::Scene *scene, const std::vector<std::string> &menuTexts, const cocos2d::Vec2 &basePos, const cocos2d::Vec2 &anchorPoint) {
    if (UNLIKELY(!Layer::init())) {
        return false;
    }

    _scene = scene;

    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float maxWidth = std::min(visibleSize.width * 0.333f, 100.0f);

    // 监听返回键
    EventListenerKeyboard *keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyReleased = [this](EventKeyboard::KeyCode keyCode, Event *event) {
        if (keyCode == EventKeyboard::KeyCode::KEY_BACK) {
            event->stopPropagation();
            this->dismiss();
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

    // 遮罩
    this->addChild(LayerColor::create(Color4B(0, 0, 0, 127)));

    // 背景
    LayerColor *background = LayerColor::create(Color4B(255, 255, 255, 245));
    this->addChild(background);
    background->setIgnoreAnchorPointForPosition(false);
    background->setPosition(basePos);
    background->setAnchorPoint(anchorPoint);

    size_t buttonsCount = menuTexts.size();
    background->setContentSize(Size(maxWidth, buttonsCount * 32.0f - 2.0f));

    const Color4B lineColor(227, 227, 227, 255);
    for (size_t i = 0, cnt = buttonsCount; i < cnt; ++i) {
        if (i > 0) {
            // 分隔线
            LayerColor *line = LayerColor::create(lineColor, maxWidth, 2.0f);
            background->addChild(line);
            line->setPosition(Vec2(0.0f, (buttonsCount - i) * 32.0f - 2.0f));
        }

        // 按钮
        const std::string &title = menuTexts[i];

        ui::Button *button = ui::Button::create();
        button->loadTexturePressed("source_material/btn_square_disabled.png");
        background->addChild(button);
        button->setScale9Enabled(true);
        button->setContentSize(Size(maxWidth, 30.0f));
        button->setTitleFontSize(12);
        button->setTitleColor(C3B_GRAY);
        button->setTitleText(title);
        button->setPosition(Vec2(maxWidth * 0.5f, (buttonsCount - i) * 32.0f - 17.0f));
        button->addClickEventListener(std::bind(&PopupMenu::onMenuButton, this, std::placeholders::_1));
        button->setUserData(reinterpret_cast<void *>(i));
    }

    // 触摸监听，点击background以外的部分按按下取消键处理
    EventListenerTouchOneByOne *touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [this, background](Touch *touch, Event *event) {
        Vec2 pos = this->convertTouchToNodeSpace(touch);
        if (background->getBoundingBox().containsPoint(pos)) {
            return true;
        }
        event->stopPropagation();
        this->dismiss();
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
    _touchListener = touchListener;

    return true;
}

void PopupMenu::onMenuButton(cocos2d::Ref *sender) {
    if (_onMenuItemCallback) {
        size_t idx = reinterpret_cast<size_t>(((ui::Button *)sender)->getUserData());
        this->retain();
        _onMenuItemCallback(this, idx);
        this->release();
    }
    this->dismiss();
}
