#include "AlertDialog.h"
#include "../UICommon.h"
#include "../UIColors.h"
#include "../cocos-wheels/CWCommon.h"

USING_NS_CC;

AlertDialog *AlertDialog::Builder::create() {
    return AlertDialog::createWithBuilder(std::move(*this));
}

AlertDialog *AlertDialog::createWithBuilder(Builder &&builder) {
    AlertDialog *ret = new (std::nothrow) AlertDialog();
    if (ret != nullptr && ret->initWithBuilder(std::move(builder))) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool AlertDialog::initWithBuilder(Builder &&builder) {
    if (UNLIKELY(!Layer::init())) {
        return false;
    }

    _scene.swap(builder._scene);
    _positiveCallback.swap(builder._positiveCallback);
    _negativeCallback.swap(builder._negativeCallback);
    _isCancelable = builder._isCancelable;
    _isCloseOnTouchOutside = builder._isCloseOnTouchOutside;

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 监听返回键
    EventListenerKeyboard *keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyReleased = [this](EventKeyboard::KeyCode keyCode, Event *event) {
        if (keyCode == EventKeyboard::KeyCode::KEY_BACK) {
            if (_isCancelable) {
                event->stopPropagation();
                onNegativeButton(nullptr);
            }
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

    // 遮罩
    this->addChild(LayerColor::create(Color4B(0, 0, 0, 127)));

    const float maxWidth1 = AlertDialog::maxWidth();
    const float totalWidth = maxWidth1 + 10.0f;

    // 背景
    LayerColor *background = LayerColor::create(Color4B(255, 255, 255, 245));
    this->addChild(background);
    background->setIgnoreAnchorPointForPosition(false);
    background->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.5f));

    float totalHeight = 0.0f;
    const std::string &positiveTitle = builder._positiveTitle;
    const std::string &negativeTitle = builder._negativeTitle;
    const bool positiveTitleEmpty = positiveTitle.empty();
    const bool negativeTitleEmpty = negativeTitle.empty();
    if (!positiveTitleEmpty || !negativeTitleEmpty) {
        if (!negativeTitleEmpty && !positiveTitleEmpty) {
            ui::Button *button = ui::Button::create("source_material/btn_square_disabled.png", "source_material/btn_square_selected.png");
            background->addChild(button);
            button->setScale9Enabled(true);
            button->setContentSize(Size(totalWidth * 0.5f, 30.0f));
            button->setTitleFontSize(14);
            button->setTitleText(negativeTitle);
            button->setPosition(Vec2(totalWidth * 0.25f, 15.0f));
            button->addClickEventListener(std::bind(&AlertDialog::onNegativeButton, this, std::placeholders::_1));

            button = UICommon::createButton();
            background->addChild(button);
            button->setScale9Enabled(true);
            button->setContentSize(Size(totalWidth * 0.5f, 30.0f));
            button->setTitleFontSize(14);
            button->setTitleText(positiveTitle);
            button->setPosition(Vec2(totalWidth * 0.75f, 15.0f));
            button->addClickEventListener(std::bind(&AlertDialog::onPositiveButton, this, std::placeholders::_1));

            totalHeight += 30.0f;
        }
        else if (!positiveTitleEmpty) {
            // 分隔线
            LayerColor *line = LayerColor::create(Color4B(227, 227, 227, 255), totalWidth, 2.0f);
            background->addChild(line);
            line->setPosition(Vec2(0.0f, totalHeight + 28.0f));

            ui::Button *button = UICommon::createButton();
            background->addChild(button);
            button->setScale9Enabled(true);
            button->setContentSize(Size(55.0f, 20.0f));
            button->setTitleFontSize(12);
            button->setTitleText(positiveTitle);
            button->setPosition(Vec2(totalWidth * 0.5f, 14.0f));
            button->addClickEventListener(std::bind(&AlertDialog::onPositiveButton, this, std::placeholders::_1));

            totalHeight += 30.0f;
        }
        else if (!negativeTitleEmpty) {
            // 分隔线
            LayerColor *line = LayerColor::create(Color4B(227, 227, 227, 255), totalWidth, 2.0f);
            background->addChild(line);
            line->setPosition(Vec2(0.0f, totalHeight + 28.0f));

            ui::Button *button = UICommon::createButton();
            background->addChild(button);
            button->setScale9Enabled(true);
            button->setContentSize(Size(55.0f, 20.0f));
            button->setTitleFontSize(12);
            button->setTitleText(negativeTitle);
            button->setPosition(Vec2(totalWidth * 0.5f, 14.0f));
            button->addClickEventListener(std::bind(&AlertDialog::onNegativeButton, this, std::placeholders::_1));

            totalHeight += 30.0f;
        }
    }

    Node *node = builder._contentNode;
    if (node != nullptr) {
        totalHeight += 5.0f;

        Size nodeSize = node->getContentSize();
        if (nodeSize.width > maxWidth1) {
            float scale = maxWidth1 / nodeSize.width;
            node->setScale(scale);

            nodeSize.width = maxWidth1;
            nodeSize.height *= scale;
        }

        // 传入的node
        background->addChild(node);
        node->setIgnoreAnchorPointForPosition(false);
        node->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
        node->setPosition(Vec2(totalWidth * 0.5f, totalHeight + nodeSize.height * 0.5f));

        totalHeight += nodeSize.height;
    }

    if (!builder._message.empty()) {
        totalHeight += 10.0f;

        Label *label = Label::createWithSystemFont(builder._message, "Arail", 12);
        label->setTextColor(C4B_BLACK);
        if (label->getContentSize().width > maxWidth1) {  // 当宽度超过时，设置范围，使文本换行
            label->setDimensions(maxWidth1, 0.0f);
            label->setLineSpacing(2.0f);
        }
        Size labelSize = label->getContentSize();
        background->addChild(label);
        label->setPosition(Vec2(totalWidth * 0.5f, totalHeight + labelSize.height * 0.5f));

        totalHeight += labelSize.height;
        totalHeight += 5.0f;
    }

    if (!builder._title.empty()) {
        totalHeight += 5.0f;

        // 分隔线
        LayerColor *line = LayerColor::create(Color4B(227, 227, 227, 255), totalWidth, 2.0f);
        background->addChild(line);
        line->setPosition(Vec2(0.0f, totalHeight));

        totalHeight += 2.0f;

        // 标题
        Label *label = Label::createWithSystemFont(builder._title, "Arail", 14);
        label->setTextColor(C4B_BLUE_THEME);
        background->addChild(label);
        label->setPosition(Vec2(totalWidth * 0.5f, totalHeight + 15.0f));
        cw::trimLabelStringWithEllipsisToFitWidth(label, totalWidth - 4.0f);

        totalHeight += 20.0f;
    }
    totalHeight += 10.0f;

    background->setContentSize(Size(totalWidth, totalHeight));

    // 触摸监听，点击background以外的部分按按下取消键处理
    EventListenerTouchOneByOne *touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [this, background](Touch *touch, Event *event) {
        if (_isCloseOnTouchOutside) {
            Vec2 pos = this->convertTouchToNodeSpace(touch);
            if (background->getBoundingBox().containsPoint(pos)) {
                return true;
            }
            event->stopPropagation();
            onNegativeButton(nullptr);
        }
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
    _touchListener = touchListener;

    return true;
}

void AlertDialog::onPositiveButton(cocos2d::Ref *) {
    bool shouldDismiss = true;
    if (_positiveCallback) {
        this->retain();
        shouldDismiss = _positiveCallback(this, BUTTON_POSITIVE);
        this->release();
    }

    if (shouldDismiss) {
        this->dismiss();
    }
}

void AlertDialog::onNegativeButton(cocos2d::Ref *) {
    bool shouldDismiss = true;
    if (_negativeCallback) {
        this->retain();
        shouldDismiss = _negativeCallback(this, BUTTON_NEGATIVE);
        this->release();
    }

    if (shouldDismiss) {
        this->dismiss();
    }
}
