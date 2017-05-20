#include "TilesKeyboard.h"
#include "../compiler.h"
#include "../TilesImage.h"
#include "../mahjong-algorithm/stringify.h"
#include "../mahjong-algorithm/fan_calculator.h"

USING_NS_CC;

#define BUTTON_WIDTH 60
#define BUTTON_HEIGHT 35
#define GAP 4

#define INPUT_GAP 8
#define INPUT_HEIGHT 20

#define MOVE_DURATION 0.2f

enum Keyboard {
    KEY_OK = 0,
    KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_SUIT,
    KEY_E, KEY_S, KEY_W, KEY_N, KEY_C, KEY_F, KEY_P,
    KEY_LEFT_BRACKET, KEY_SPACE, KEY_RIGHT_BRACKET, KEY_DELETE, KEY_CLEAR, KEY_GLOBAL
};

static const char *buttonFace[] = {
    "",
    "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "",
    "东 E", "南 S", "西 W", "北 N", "中 C", "发 F", "白 P",
    "[", "", "]", "", "清空", "\xF0\x9F\x8C\x90"
};

static const Keyboard keyIdx[] = {
    KEY_E, KEY_1, KEY_2, KEY_3, KEY_SUIT,
    KEY_S, KEY_4, KEY_5, KEY_6, KEY_LEFT_BRACKET,
    KEY_W, KEY_7, KEY_8, KEY_9, KEY_RIGHT_BRACKET,
    KEY_N, KEY_C, KEY_F, KEY_P, KEY_DELETE,
    KEY_GLOBAL, KEY_SPACE, KEY_SPACE, KEY_CLEAR, KEY_OK
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
    const int buttonAreaHeight = BUTTON_HEIGHT * 5 + GAP * 6;
    const int height = buttonAreaHeight + TILE_HEIGHT + GAP * 3 + 10;

    Node *rootNode = Node::create();
    rootNode->setContentSize(Size(width, height));
    rootNode->setAnchorPoint(Vec2::ANCHOR_MIDDLE);

    // 背景
    LayerColor *background = LayerColor::create(Color4B(238, 238, 238, 0xFF), width, buttonAreaHeight);
    rootNode->addChild(background);

    background = LayerColor::create(Color4B(224, 224, 224, 0xFF), width, TILE_HEIGHT + GAP * 3 + 10);
    rootNode->addChild(background);
    background->setPosition(Vec2(0, buttonAreaHeight));

    // 牌数量label
    Label *label = Label::createWithSystemFont("当前牌数目：0", "Arial", 10);
    rootNode->addChild(label);
    label->setColor(Color3B::BLACK);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(INPUT_GAP, buttonAreaHeight + TILE_HEIGHT + GAP * 2 + 5));
    _countLabel = label;

    // 牌的根结点
    _tilesContainer = Node::create();
    _tilesContainer->setIgnoreAnchorPointForPosition(false);
    _tilesContainer->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    rootNode->addChild(_tilesContainer);
    _tilesContainer->setPosition(Vec2(width * 0.5f, buttonAreaHeight + GAP + TILE_HEIGHT * 0.5f));

    // 排列按钮
    ui::Button *buttons[25];
    for (int i = 0; i < 25; ++i) {
        Keyboard key = keyIdx[i];
        div_t ret = div(i, 5);
        int x = ret.rem;
        int y = ret.quot;
        ui::Button *button = ui::Button::create("source_material/tabBar_selected_bg.png");
        rootNode->addChild(button);
        button->setScale9Enabled(true);
        button->setTitleFontSize(18);
        button->setTitleColor(Color3B::BLACK);
        button->setTitleText(buttonFace[key]);
        button->setContentSize(Size(BUTTON_WIDTH, BUTTON_HEIGHT));
        button->setPosition(Vec2(BUTTON_WIDTH * (x + 0.5f) + GAP * (x + 1), BUTTON_HEIGHT * (5 - y - 0.5f) + GAP * (5 - y)));
        button->setUserData(reinterpret_cast<void *>(key));
        button->addClickEventListener(std::bind(&TilesKeyboard::onKeyboardButton, this, std::placeholders::_1));
        buttons[i] = button;
    }

    // 返回键
    ui::Button *button = buttons[24];
    button->loadTextureNormal("source_material/btn_square_highlighted.png");
    button->setTitleColor(Color3B::WHITE);
    Sprite *sprite = Sprite::create("drawable/sym_keyboard_return.png");
    sprite->setPosition(Vec2(BUTTON_WIDTH * 0.5f, BUTTON_HEIGHT * 0.5f));
    sprite->setScale(30 / sprite->getContentSize().height);
    button->getRendererNormal()->addChild(sprite);

    // 花色
    _suitButton = buttons[4];
    _suitButton->loadTextureNormal("source_material/btn_square_highlighted.png");
    _suitButton->setTitleColor(Color3B::WHITE);

    // 删除键
    sprite = Sprite::create("drawable/sym_keyboard_feedback_delete.png");
    sprite->setPosition(Vec2(BUTTON_WIDTH * 0.5f, BUTTON_HEIGHT * 0.5f));
    sprite->setScale(30 / sprite->getContentSize().height);
    buttons[19]->getRendererNormal()->addChild(sprite);

    // 多余的空格键
    buttons[22]->removeFromParent();

    // 空格键
    button = buttons[21];
    button->setContentSize(Size(BUTTON_WIDTH * 2 + GAP, BUTTON_HEIGHT));
    button->setPosition(Vec2((BUTTON_WIDTH + GAP) * 0.5f, 0) + button->getPosition());
    sprite = Sprite::create("drawable/sym_keyboard_feedback_space.png");
    sprite->setPosition(Vec2(BUTTON_WIDTH + GAP * 0.5f, BUTTON_HEIGHT * 0.5f));
    sprite->setScale(30 / sprite->getContentSize().height);
    button->getRendererNormal()->addChild(sprite);

    // 输入文本的背景
    LayerColor *inputBg = LayerColor::create(Color4B(238, 238, 238, 238), 0, INPUT_HEIGHT);
    _inputBg = inputBg;
    rootNode->addChild(inputBg);
    inputBg->setPosition(Vec2(0, height));

    // 输入文本
    label = Label::createWithSystemFont("", "Arial", 12);
    inputBg->addChild(label);
    label->setColor(Color3B::BLACK);
    label->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    label->setPosition(Vec2(INPUT_GAP, INPUT_HEIGHT / 2));
    _inputLabel = label;

    // 缩放
    this->addChild(rootNode);
    Size rootSize = rootNode->getContentSize();
    if (rootSize.width > visibleSize.width) {
        const float scale = visibleSize.width / rootSize.width;
        rootNode->setScale(scale);
        rootSize.width = visibleSize.width;
        rootSize.height *= scale;
    }
    else {
        rootSize.width = visibleSize.width;
    }

    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    rootNode->setPosition(Vec2(origin.x + rootSize.width * 0.5f, origin.y + rootSize.height * 0.5f));
    _rootNode = rootNode;

    // 触摸监听，点击background以外的部分按按下取消键处理
    EventListenerTouchOneByOne *touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [this](Touch *touch, Event *event) {
        Vec2 pos = this->convertTouchToNodeSpace(touch);
        if (_rootNode->getBoundingBox().containsPoint(pos)) {
            return true;
        }
        event->stopPropagation();
        dismissWithEvent(DismissEvent::CANCEL);
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
    _touchListener = touchListener;

    _tilesSprite.reserve(18);

    _currentSuit = 0;
    _suitButton->setTitleText("万 m");
    return true;
}

void TilesKeyboard::onKeyboardButton(cocos2d::Ref *sender) {
    Keyboard n = (Keyboard)reinterpret_cast<intptr_t>(((ui::Button *)sender)->getUserData());
    switch (n) {
    case KEY_1: case KEY_2: case KEY_3:
    case KEY_4: case KEY_5: case KEY_6:
    case KEY_7: case KEY_8: case KEY_9: {
        std::string temp = _tilesText;
        _tilesText.append(1, '0' + n);
        onNumberedSuffix(_currentSuit);

        // 合并后缀
        if (!temp.empty() && "msp"[_currentSuit] == temp.back()) {
            size_t len = temp.length();
            _tilesText.erase(_tilesText.begin() + len - 1, _tilesText.begin() + len);
            refreshInputLabel();
        }
        break;
    }
    case KEY_SUIT:
        ++_currentSuit;
        if (_currentSuit > 2) _currentSuit = 0;
        switch (_currentSuit) {
        case 0: _suitButton->setTitleText("万 m"); break;
        case 1: _suitButton->setTitleText("条 s"); break;
        case 2: _suitButton->setTitleText("饼 p"); break;
        default: break;
        }
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
    case KEY_CLEAR:
        onClear();
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

void TilesKeyboard::refreshInputLabel() {
    _inputLabel->setString(_tilesText);
    if (UNLIKELY(_tilesText.empty())) {
        _inputBg->setContentSize(Size(0, INPUT_HEIGHT));
    }
    else {
        _inputBg->setContentSize(Size(INPUT_GAP * 2 + _inputLabel->getContentSize().width, INPUT_HEIGHT));
    }

    _onTextChange(_tilesText.c_str());
}

void TilesKeyboard::setTilesText(const char *text) {
    for (char ch = *text; ch != '\0'; ++text, ch = *text) {
        switch (ch) {
        case '1': case '2': case '3':
        case '4': case '5': case '6':
        case '7': case '8': case '9':
            _tilesText.append(1, ch);
            refreshInputLabel();
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
    _rootNode->runAction(
        MoveBy::create(MOVE_DURATION, Vec2(0, -_rootNode->getContentSize().height * _rootNode->getScale())));
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
        sprite->setTag(tiles[i]);
    }

    containerSize.width += TILE_WIDTH * count;
    containerSize.height = TILE_HEIGHT;
    _tilesContainer->setContentSize(containerSize);
    const float maxWidth = _tilesContainer->getParent()->getContentSize().width;
    if (containerSize.width > maxWidth) {
        const float scale = maxWidth / containerSize.width;
        _tilesContainer->setScale(scale);
    }

    _countLabel->setString(StringUtils::format("当前牌数目：%lu",
        (unsigned long)std::count_if(_tilesSprite.begin(), _tilesSprite.end(), [](Sprite *s) {
        return s->getTag() != INVALID_TAG;
    })));
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

    _countLabel->setString(StringUtils::format("当前牌数目：%lu",
        (unsigned long)std::count_if(_tilesSprite.begin(), _tilesSprite.end(), [](Sprite *s) {
        return s->getTag() != INVALID_TAG;
    })));
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
    refreshInputLabel();

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
    refreshInputLabel();

    mahjong::tile_t tile = mahjong::TILE_E + honor;
    addTiles(&tile, 1);
}

void TilesKeyboard::onBackspace() {
    if (_tilesText.empty()) {
        return;
    }

    char ch = _tilesText.back();
    _tilesText.pop_back();

    switch (ch) {
    case 'm': case 's': case 'p': {
        _tilesText.pop_back();
        if (!_tilesText.empty() && isdigit(_tilesText.back())) {
            _tilesText.push_back(ch);
        }
        removeTiles(1);
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

    refreshInputLabel();
}

void TilesKeyboard::onSpace() {
    if (_tilesText.empty() || isdigit(_tilesText.back()) || _tilesText.back() == ' ') {
        return;
    }

    _tilesText.append(1, ' ');
    refreshInputLabel();

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
    refreshInputLabel();

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
    refreshInputLabel();

    addSpaceTile();
}

void TilesKeyboard::onClear() {
    _tilesText.clear();
    refreshInputLabel();

    _tilesContainer->removeAllChildren();
    _tilesSprite.clear();

    _tilesContainer->setContentSize(Size(0, TILE_HEIGHT));
    _tilesContainer->setScale(1.0f);

    _countLabel->setString("当前牌数目：0");
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
    Vec2 keyboardTopPos = _rootNode->convertToWorldSpace(_rootNode->getContentSize());
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

const char *TilesKeyboard::parseInput(const char *input, const std::function<void (const mahjong::hand_tiles_t &, mahjong::tile_t)> &callback) {
    if (*input == '\0') {
        return nullptr;
    }

    mahjong::hand_tiles_t hand_tiles;
    mahjong::tile_t win_tile;
    long ret = mahjong::string_to_tiles(input, &hand_tiles, &win_tile);
    if (ret != PARSE_NO_ERROR) {
        switch (ret) {
        case PARSE_ERROR_ILLEGAL_CHARACTER: return "无法解析的字符";
        case PARSE_ERROR_NO_SUFFIX_AFTER_DIGIT: return "数字后面需有后缀";
        case PARSE_ERROR_WRONG_TILES_COUNT_FOR_FIXED_PACK: return "一组副露包含了错误的牌数目";
        case PARSE_ERROR_CANNOT_MAKE_FIXED_PACK: return "无法正确解析副露";
        default: break;
        }
        return nullptr;
    }
    if (win_tile == 0) {
        return "缺少和牌张";
    }

    ret = mahjong::check_calculator_input(&hand_tiles, win_tile);
    if (ret != 0) {
        switch (ret) {
        case ERROR_WRONG_TILES_COUNT: return "牌张数错误";
        case ERROR_TILE_COUNT_GREATER_THAN_4: return "同一种牌最多只能使用4枚";
        default: break;
        }
        return nullptr;
    }

    callback(hand_tiles, win_tile);
    return nullptr;
}
