#include "TilesKeyboard.h"
#include "../compiler.h"
#include "../TilesImage.h"

USING_NS_CC;

#define BUTTON_WIDTH 60
#define BUTTON_HEIGHT 35
#define GAP 4

#define MOVE_DURATION 0.2f

enum Keyboard {
    KEY_OK = 0,
    KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_m, KEY_s, KEY_p,
    KEY_E, KEY_S, KEY_W, KEY_N, KEY_C, KEY_F, KEY_P,
    KEY_LEFT_BRACKET, KEY_SPACE, KEY_RIGHT_BRACKET, KEY_DELETE, KEY_GLOBAL
};

static const char *buttonFace[] = {
    "\xE2\x86\xB5",
    "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "万 m", "条 s", "饼 p",
    "东 E", "南 S", "西 W", "北 N", "中 C", "发 F", "白 P",
    "[", "空格", "]", "\xE2\x9D\x8C", "\xF0\x9F\x8C\x90"
};

static const Keyboard keyIdx[] = {
    KEY_E, KEY_1, KEY_2, KEY_3, KEY_m,
    KEY_S, KEY_4, KEY_5, KEY_6, KEY_s,
    KEY_W, KEY_7, KEY_8, KEY_9, KEY_p,
    KEY_N, KEY_C, KEY_F, KEY_P, KEY_DELETE,
    KEY_GLOBAL, KEY_LEFT_BRACKET, KEY_SPACE, KEY_RIGHT_BRACKET, KEY_OK
};

bool TilesKeyboard::init() {
    if (UNLIKELY(!Layer::init())) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();

    // 监听返回键
    EventListenerKeyboard *keyboardListener = EventListenerKeyboard::create();
    keyboardListener->onKeyReleased = [this](EventKeyboard::KeyCode keyCode, Event *event) {
        if (keyCode == EventKeyboard::KeyCode::KEY_BACK) {
            event->stopPropagation();
            dismissWithEvent(DismissEvent::CANCEL);
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

    const int width = BUTTON_WIDTH * 5 + GAP * 6;
    const int buttonAreaHeight = BUTTON_HEIGHT * 5 + GAP * 7;
    const int height = buttonAreaHeight + TILE_HEIGHT;

    // 背景
    LayerColor *background = LayerColor::create(Color4B(38, 50, 56, 0xFF), width, height);
    background->setIgnoreAnchorPointForPosition(false);

    // 牌的根结点
    _tilesContainer = Node::create();
    _tilesContainer->setIgnoreAnchorPointForPosition(false);
    _tilesContainer->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    background->addChild(_tilesContainer);
    _tilesContainer->setPosition(Vec2(width * 0.5f, height - TILE_HEIGHT * 0.5f - GAP));

    // 排列按钮
    for (int i = 0; i < 25; ++i) {
        Keyboard key = keyIdx[i];
        div_t ret = div(i, 5);
        int x = ret.rem;
        int y = ret.quot;
        ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
        background->addChild(button);
        button->setScale9Enabled(true);
        button->setTitleFontSize(20);
        button->setTitleText(buttonFace[key]);
        button->setContentSize(Size(BUTTON_WIDTH, BUTTON_HEIGHT));
        button->setPosition(Vec2(BUTTON_WIDTH * (x + 0.5f) + GAP * (x + 1), BUTTON_HEIGHT * (5 - y - 0.5f) + GAP * (5 - y)));
        button->setUserData(reinterpret_cast<void *>(key));
        button->addClickEventListener(std::bind(&TilesKeyboard::onKeyboardButton, this, std::placeholders::_1));
    }

    // 缩放背景
    this->addChild(background);
    Size bgSize = background->getContentSize();
    if (bgSize.width > visibleSize.width) {
        const float scale = visibleSize.width / bgSize.width;
        background->setScale(scale);
        bgSize.width = visibleSize.width;
        bgSize.height *= scale;
    }
    else {
        bgSize.width = visibleSize.width;
    }

    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    background->setPosition(Vec2(origin.x + bgSize.width * 0.5f, origin.y + bgSize.height * 0.5f));
    _background = background;

    // 触摸监听，点击background以外的部分按按下取消键处理
    EventListenerTouchOneByOne *touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [this](Touch *touch, Event *event) {
        Vec2 pos = this->convertTouchToNodeSpace(touch);
        if (_background->getBoundingBox().containsPoint(pos)) {
            return true;
        }
        event->stopPropagation();
        dismissWithEvent(DismissEvent::CANCEL);
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
    _touchListener = touchListener;

    _tilesSprite.reserve(18);

    return true;
}

void TilesKeyboard::onKeyboardButton(cocos2d::Ref *sender) {
    Keyboard n = (Keyboard)reinterpret_cast<intptr_t>(((ui::Button *)sender)->getUserData());
    switch (n) {
    case KEY_1: case KEY_2: case KEY_3:
    case KEY_4: case KEY_5: case KEY_6:
    case KEY_7: case KEY_8: case KEY_9: {
        _tilesText.append(1, '0' + n);
        _onTextChange(_tilesText.c_str());
        break;
    }
    case KEY_m: case KEY_s: case KEY_p:
        onNumberedSuffix(n - KEY_m);
        break;
    case KEY_E: case KEY_S: case KEY_W: case KEY_N:
    case KEY_C: case KEY_F: case KEY_P:
        onHonor(n - KEY_E);
        break;
    case KEY_DELETE:
        onBackspace();
        break;
    case KEY_SPACE:
        onSpace();
        break;
    case KEY_LEFT_BRACKET:
        onLeftBracket();
        break;
    case KEY_RIGHT_BRACKET:
        onRightBracket();
        break;
    case KEY_OK:
        dismissWithEvent(DismissEvent::OK);
        break;
    case KEY_GLOBAL:
        dismissWithEvent(DismissEvent::GLOBAL);
        break;
    default:
        dismissWithEvent(DismissEvent::CANCEL);
        break;
    }
}

void TilesKeyboard::setTilesText(const char *text) {
    for (char ch = *text; ch != '\0'; ++text, ch = *text) {
        switch (ch) {
        case '1': case '2': case '3':
        case '4': case '5': case '6':
        case '7': case '8': case '9':
            _tilesText.append(1, ch);
            _onTextChange(_tilesText.c_str());
            break;
        case 'm': onNumberedSuffix(0); break;
        case 's': onNumberedSuffix(1); break;
        case 'p': onNumberedSuffix(2); break;
        case 'E': onHonor(0); break;
        case 'S': onHonor(1); break;
        case 'W': onHonor(2); break;
        case 'N': onHonor(3); break;
        case 'C': onHonor(4); break;
        case 'F': onHonor(5); break;
        case 'P': onHonor(6); break;
        case ' ': onSpace(); break;
        case '[': onLeftBracket(); break;
        case ']': onRightBracket(); break;
        default: break;
        }
    }
}

void TilesKeyboard::dismissWithEvent(DismissEvent event) {
    if (this->getNumberOfRunningActions() > 0) {
        return;
    }

    _onDismiss(event);
    _background->runAction(
        MoveBy::create(MOVE_DURATION, Vec2(0, -_background->getContentSize().height * _background->getScale())));
    this->runAction(Sequence::create(DelayTime::create(MOVE_DURATION), RemoveSelf::create(), nullptr));
}

void TilesKeyboard::addTiles(const mahjong::tile_t *tiles, size_t count) {
    const float contentScaleFactor = CC_CONTENT_SCALE_FACTOR();
    Size containerSize = _tilesContainer->getContentSize();
    for (size_t i = 0; i < count; ++i) {
        Sprite *sprite = Sprite::create(tilesImageName[tiles[i]]);
        sprite->setScale(contentScaleFactor);
        sprite->setColor(_inBracket ? Color3B(51, 204, 255) : Color3B::WHITE);
        _tilesContainer->addChild(sprite);
        sprite->setPosition(Vec2(containerSize.width + TILE_WIDTH * (i + 0.5f), TILE_HEIGHT * 0.5f));
        _tilesSprite.push_back(sprite);
    }

    containerSize.width += TILE_WIDTH * count;
    containerSize.height = TILE_HEIGHT;
    _tilesContainer->setContentSize(containerSize);
    const float maxWidth = _tilesContainer->getParent()->getContentSize().width;
    if (containerSize.width > maxWidth) {
        const float scale = maxWidth / containerSize.width;
        _tilesContainer->setScale(scale);
    }
}

void TilesKeyboard::removeTiles(size_t count) {
    Size containerSize = _tilesContainer->getContentSize();

    for (size_t i = 0; i < count; ++i) {
        Sprite *sprite = _tilesSprite.back();
        _tilesContainer->removeChild(sprite);
        _tilesSprite.pop_back();
    }

    containerSize.width -= TILE_WIDTH * count;
    containerSize.height = TILE_HEIGHT;

    _tilesContainer->setContentSize(containerSize);
    const float maxWidth = _tilesContainer->getParent()->getContentSize().width;
    if (containerSize.width > maxWidth) {
        const float scale = maxWidth / containerSize.width;
        _tilesContainer->setScale(scale);
    }
    else {
        _tilesContainer->setScale(1.0f);
    }
}

void TilesKeyboard::addSpaceTile() {
    Size containerSize = _tilesContainer->getContentSize();
    Sprite *sprite = Sprite::create();
    sprite->setVisible(false);
    _tilesContainer->addChild(sprite);
    _tilesSprite.push_back(sprite);

    containerSize.width += GAP;
    containerSize.height = TILE_HEIGHT;
    _tilesContainer->setContentSize(containerSize);
    const float maxWidth = _tilesContainer->getParent()->getContentSize().width;
    if (containerSize.width > maxWidth) {
        const float scale = maxWidth / containerSize.width;
        _tilesContainer->setScale(scale);
    }
}

void TilesKeyboard::removeSpaceTile() {
    Size containerSize = _tilesContainer->getContentSize();

    Sprite *sprite = _tilesSprite.back();
    _tilesContainer->removeChild(sprite);
    _tilesSprite.pop_back();

    containerSize.width -= GAP;
    containerSize.height = TILE_HEIGHT;

    _tilesContainer->setContentSize(containerSize);
    const float maxWidth = _tilesContainer->getParent()->getContentSize().width;
    if (containerSize.width > maxWidth) {
        const float scale = maxWidth / containerSize.width;
        _tilesContainer->setScale(scale);
    }
    else {
        _tilesContainer->setScale(1.0f);
    }
}

void TilesKeyboard::onNumberedSuffix(int suit) {
    if (_tilesText.empty() || !isdigit(_tilesText.back())) {
        return;
    }

    std::string::size_type len = _tilesText.length();
    std::string::size_type lastSuffixPos
        = std::find_if_not(_tilesText.rbegin(), _tilesText.rend(), isdigit).base() - _tilesText.begin();

    const char *suitFace = "msp";
    _tilesText.append(1, suitFace[suit]);
    _onTextChange(_tilesText.c_str());

    std::vector<mahjong::tile_t> tiles;
    for (size_t i = lastSuffixPos; i < len; ++i) {
        tiles.push_back(mahjong::make_tile(suit + 1, _tilesText[i] - '0'));
    }
    addTiles(&tiles.front(), tiles.size());
}

void TilesKeyboard::onHonor(int honor) {
    if (!_tilesText.empty() && isdigit(_tilesText.back())) {
        return;
    }

    const char *honorFace = "ESWNCFP";
    _tilesText.append(1, honorFace[honor]);
    _onTextChange(_tilesText.c_str());

    mahjong::tile_t tile = mahjong::TILE_E + honor;
    addTiles(&tile, 1);
}

void TilesKeyboard::onBackspace() {
    if (_tilesText.empty()) {
        return;
    }

    char ch = _tilesText.back();
    _tilesText.pop_back();
    _onTextChange(_tilesText.c_str());

    switch (ch) {
    case 'm': case 's': case 'p': {
        std::string::size_type len = _tilesText.length();
        std::string::size_type lastSuffixPos
            = std::find_if_not(_tilesText.rbegin(), _tilesText.rend(), isdigit).base() - _tilesText.begin();

        removeTiles(len - lastSuffixPos);
        break;
    }
    case 'E': case 'S': case 'W': case 'N': case 'C': case 'F': case 'P':
        removeTiles(1);
        break;
    case '[':
        _inBracket = false;
        removeSpaceTile();
        break;
    case ']':
        _inBracket = true;
        removeSpaceTile();
        break;
    case ' ':
        removeSpaceTile();
        break;
    default:
        break;
    }
}

void TilesKeyboard::onSpace() {
    if (_tilesText.empty() || isdigit(_tilesText.back()) || _tilesText.back() == ' ') {
        return;
    }

    _tilesText.append(1, ' ');
    _onTextChange(_tilesText.c_str());

    addSpaceTile();
}

void TilesKeyboard::onLeftBracket() {
    if (_inBracket) {
        return;
    }

    if (!_tilesText.empty() && isdigit(_tilesText.back())) {
        return;
    }

    _inBracket = true;
    _tilesText.append(1, '[');
    _onTextChange(_tilesText.c_str());

    addSpaceTile();
}

void TilesKeyboard::onRightBracket() {
    if (!_inBracket) {
        return;
    }

    if (!_tilesText.empty() && isdigit(_tilesText.back())) {
        return;
    }

    _inBracket = false;
    _tilesText.append(1, ']');
    _onTextChange(_tilesText.c_str());

    addSpaceTile();
}

void TilesKeyboard::hookEditBox(cocos2d::ui::EditBox *editBox) {
    const Size &size = editBox->getContentSize();
    ui::Button *button = ui::Button::create();
    button->setScale9Enabled(true);
    button->setContentSize(size);
    button->addClickEventListener([editBox](Ref *) {
        TilesKeyboard::showByEditBox(editBox);
    });

    editBox->addChild(button);
    button->setPosition(Vec2(size.width * 0.5f, size.height * 0.5f));
}

void TilesKeyboard::showByEditBox(cocos2d::ui::EditBox *editBox) {
    TilesKeyboard *keyboard = TilesKeyboard::create();
    keyboard->_onTextChange = std::bind(&ui::EditBox::setText, editBox, std::placeholders::_1);

    std::string originText = editBox->getText();
    keyboard->setTilesText(originText.c_str());

    Director::getInstance()->getRunningScene()->addChild(keyboard, 150);

    keyboard->associateWithEditBox(editBox);
}

void TilesKeyboard::associateWithEditBox(cocos2d::ui::EditBox *editBox) {
    Vec2 keyboardTopPos = _background->convertToWorldSpace(_background->getContentSize());
    Vec2 editBottomPos = editBox->convertToWorldSpace(Vec2::ZERO);
    float delta = editBottomPos.y - keyboardTopPos.y;
    Node *movedNode = editBox->getParent();
    if (delta < 0) {
        Scene *scene = Director::getInstance()->getRunningScene();
        for (Node *temp = movedNode->getParent(); temp != scene; temp = temp->getParent()) {
            movedNode = temp;
        }
        movedNode->runAction(MoveBy::create(MOVE_DURATION, Vec2(0, -delta)));
    }

    _onDismiss = [editBox, delta, movedNode](DismissEvent event) {
        if (delta < 0) {
            movedNode->runAction(MoveBy::create(MOVE_DURATION, Vec2(0, delta)));
        }

        if (event == DismissEvent::GLOBAL) {
            editBox->touchDownAction(editBox, ui::Widget::TouchEventType::ENDED);
        }
        else {
            ui::EditBoxDelegate *delegate = editBox->getDelegate();
            if (delegate != nullptr) {
                delegate->editBoxReturn(editBox);
            }
        }
    };
}
