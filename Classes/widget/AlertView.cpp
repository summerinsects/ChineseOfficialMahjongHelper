#include "AlertView.h"
#include "ui/CocosGUI.h"
#include "../utils/common.h"

USING_NS_CC;

void AlertView::showWithNode(const std::string &title, cocos2d::Node *node, float maxWidth, const std::function<void ()> &confirmCallback, const std::function<void ()> &cancelCallback) {
    AlertView *alert = new (std::nothrow) AlertView();
    if (alert != nullptr && alert->initWithTitle(title, node, maxWidth, confirmCallback, cancelCallback)) {
        alert->autorelease();
        Director::getInstance()->getRunningScene()->addChild(alert, 100);
        return;
    }
    CC_SAFE_DELETE(alert);
}

void AlertView::showWithMessage(const std::string &title, const std::string &message, float fontSize, const std::function<void ()> &confirmCallback, const std::function<void ()> &cancelCallback) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float maxWidth = visibleSize.width * 0.8f - 10.0f;
    Label *label = Label::createWithSystemFont(message, "Arail", fontSize);
    label->setColor(Color3B::BLACK);
    if (label->getContentSize().width > maxWidth) {  // 当宽度超过时，设置范围，使文本换行
        label->setDimensions(maxWidth, 0.0f);
    }
    AlertView::showWithNode(title, label, maxWidth, confirmCallback, cancelCallback);
}

bool AlertView::initWithTitle(const std::string &title, cocos2d::Node *node, float maxWidth, const std::function<void ()> &confirmCallback, const std::function<void ()> &cancelCallback) {
    if (UNLIKELY(!Layer::init())) {
        return false;
    }

    _confirmCallback = confirmCallback;
    _cancelCallback = cancelCallback;

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 监听返回键
    EventListenerKeyboard *keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyReleased = [this](EventKeyboard::KeyCode keyCode, Event *event) {
        if (keyCode == EventKeyboard::KeyCode::KEY_BACK) {
            event->stopPropagation();
            onCancelButton(nullptr);
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

    // 遮罩
    this->addChild(LayerColor::create(Color4B(0, 0, 0, 127)));

    const float width = maxWidth + 10.0f;

    Size nodeSize = node->getContentSize();
    if (nodeSize.width > maxWidth) {
        float scale = maxWidth / nodeSize.width;
        node->setScale(scale);

        nodeSize.width = maxWidth;
        nodeSize.height *= scale;
    }
    const float height = nodeSize.height + 78.0f;

    // 背景
    LayerColor *background = LayerColor::create(Color4B::WHITE, width, height);
    this->addChild(background);
    background->setIgnoreAnchorPointForPosition(false);
    background->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f));

    // 标题
    Label *label = Label::createWithSystemFont(title, "Arail", 14);
    label->setColor(Color3B(51, 204, 255));
    background->addChild(label);
    label->setPosition(Vec2(width * 0.5f, 62.0f + nodeSize.height));
    Common::trimLabelStringWithEllipsisToFitWidth(label, width - 4.0f);

    // 分隔线
    LayerColor *line = LayerColor::create(Color4B(227, 227, 227, 255), width, 2.0f);
    background->addChild(line);
    line->setPosition(Vec2(0.0f, 45.0f + nodeSize.height));

#if 0  // for test
    LayerColor *nodebg = LayerColor::create(Color4B::RED, nodeSize.width, nodeSize.height);
    background->addChild(nodebg);
    nodebg->ignoreAnchorPointForPosition(false);
    nodebg->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    nodebg->setPosition(Vec2(width * 0.5f, 35.0f + nodeSize.height * 0.5f));
#endif

    // 传入的node
    background->addChild(node);
    node->setIgnoreAnchorPointForPosition(false);
    node->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    node->setPosition(Vec2(width * 0.5f, 35.0f + nodeSize.height * 0.5f));

    // 取消按钮
    ui::Button *button = ui::Button::create("source_material/btn_square_disabled.png", "source_material/btn_square_selected.png");
    background->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(width * 0.5f, 30.0f));
    button->setTitleFontSize(14);
    button->setTitleText("取消");
    button->setPosition(Vec2(width * 0.25f, 10.0f));
    button->addClickEventListener(std::bind(&AlertView::onCancelButton, this, std::placeholders::_1));

    // 确定按钮
    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    background->addChild(button);
    button->setScale9Enabled(true);
    button->setContentSize(Size(width * 0.5f, 30.0f));
    button->setTitleFontSize(14);
    button->setTitleText("确定");
    button->setPosition(Vec2(width * 0.75f, 10.0f));
    button->addClickEventListener(std::bind(&AlertView::onConfirmButton, this, std::placeholders::_1));

    // 触摸监听，点击background以外的部分按按下取消键处理
    EventListenerTouchOneByOne *touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [this, background](Touch *touch, Event *event) {
        Vec2 pos = this->convertTouchToNodeSpace(touch);
        if (background->getBoundingBox().containsPoint(pos)) {
            return true;
        }
        event->stopPropagation();
        onCancelButton(nullptr);
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
    _touchListener = touchListener;

    return true;
}

void AlertView::onCancelButton(cocos2d::Ref *sender) {
    if (_cancelCallback) {
        _cancelCallback();
    }
    this->removeFromParent();
}

void AlertView::onConfirmButton(cocos2d::Ref *sender) {
    if (_confirmCallback) {
        _confirmCallback();
    }
    this->removeFromParent();
}
