#include "TilesKeyboard.h"
#include "../compiler.h"
#include "../TilesImage.h"

USING_NS_CC;

#define BUTTON_WIDTH 60
#define BUTTON_HEIGHT 35
#define GAP 4

enum Keyboard {
    KEY_OK = 0,
    KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_m, KEY_s, KEY_p,
    KEY_E, KEY_S, KEY_W, KEY_N, KEY_C, KEY_F, KEY_P,
    KEY_SPACE, KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET, KEY_DELETE
};

static const char *buttonFace[] = {
    "OK",
    "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "m", "s", "p",
    "E", "S", "W", "N", "C", "F", "P",
    "_", "[", "]", "\xE2\x9D\x8C"
};

static const Keyboard keyIdx[] = {
    KEY_E, KEY_1, KEY_2, KEY_3, KEY_m,
    KEY_S, KEY_4, KEY_5, KEY_6, KEY_s,
    KEY_W, KEY_7, KEY_8, KEY_9, KEY_p,
    KEY_N, KEY_C, KEY_F, KEY_P, KEY_DELETE,
    KEY_SPACE, KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET, KEY_OK
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
            this->removeFromParent();
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(keyboardListener, this);

    const int width = BUTTON_WIDTH * 5 + GAP * 6;
    const int buttonAreaHeight = BUTTON_HEIGHT * 5 + GAP * 6;
    const int height = buttonAreaHeight + 12 + TILE_HEIGHT;

    // 背景
    LayerColor *background = LayerColor::create(Color4B::GRAY, width, height);
    background->setIgnoreAnchorPointForPosition(false);

    // 文本
    _textLabel = Label::createWithSystemFont("", "Arial", 10);
    background->addChild(_textLabel);
    _textLabel->setPosition(Vec2(width * 0.5f, height - TILE_HEIGHT - 6));

    // 牌的根结点
    _tilesContainer = Node::create();
    _tilesContainer->setIgnoreAnchorPointForPosition(false);
    _tilesContainer->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    background->addChild(_tilesContainer);
    _tilesContainer->setPosition(Vec2(width * 0.5f, height - TILE_HEIGHT * 0.5f));

    // 排列按钮
    for (int i = 0; i < 24; ++i) {
        Keyboard key = keyIdx[i];
        div_t ret = div(i, 5);
        int x = ret.rem;
        int y = ret.quot;
        ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
        background->addChild(button);
        button->setScale9Enabled(true);
        button->setTitleFontSize(20);
        button->setTitleText(buttonFace[key]);
        if (LIKELY(i != 23)) {
            button->setContentSize(Size(BUTTON_WIDTH, BUTTON_HEIGHT));
            button->setPosition(Vec2(BUTTON_WIDTH * (x + 0.5f) + GAP * (x + 1), BUTTON_HEIGHT * (5 - y - 0.5f) + GAP * (5 - y)));
        }
        else {
            button->setContentSize(Size(BUTTON_WIDTH * 2 + GAP, BUTTON_HEIGHT));
            button->setPosition(Vec2(BUTTON_WIDTH * 4 + GAP * 4.5f, BUTTON_HEIGHT * 0.5f + GAP));
        }
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

    // 触摸监听，点击background以外的部分按按下取消键处理
    EventListenerTouchOneByOne *touchListener = EventListenerTouchOneByOne::create();
    touchListener->setSwallowTouches(true);
    touchListener->onTouchBegan = [this, background](Touch *touch, Event *event) {
        Vec2 pos = this->convertTouchToNodeSpace(touch);
        if (background->getBoundingBox().containsPoint(pos)) {
            return true;
        }
        event->stopPropagation();
        this->removeFromParent();
        return true;
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);
    _touchListener = touchListener;

    _tilesSprite.reserve(18);

    return true;
}

void TilesKeyboard::onKeyboardButton(cocos2d::Ref *sender) {
    const std::string &originText = _textLabel->getString();
    Keyboard n = (Keyboard)reinterpret_cast<intptr_t>(((ui::Button *)sender)->getUserData());
    switch (n) {
    case KEY_1: case KEY_2: case KEY_3:
    case KEY_4: case KEY_5: case KEY_6:
    case KEY_7: case KEY_8: case KEY_9: {
        std::string str = originText;
        str.append(1, '0' + n);
        _textLabel->setString(str);
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
        if (_returnCallback) {
            _returnCallback(_textLabel->getString());
        }
        this->removeFromParent();
        break;
    default:
        break;
    }
}

void TilesKeyboard::addTiles(const mahjong::tile_t *tiles, size_t count) {
    const float contentScaleFactor = CC_CONTENT_SCALE_FACTOR();
    Size containerSize = _tilesContainer->getContentSize();
    for (size_t i = 0; i < count; ++i) {
        Sprite *sprite = Sprite::create(tilesImageName[tiles[i]]);
        sprite->setScale(contentScaleFactor);
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
    const std::string &originText = _textLabel->getString();
    if (originText.empty() || !isdigit(originText.back())) {
        return;
    }

    std::string::size_type len = originText.length();
    std::string::size_type lastSuffixPos
        = std::find_if_not(originText.rbegin(), originText.rend(), isdigit).base() - originText.begin();

    const char *suitFace = "msp";
    std::string str = originText;
    str.append(1, suitFace[suit]);
    _textLabel->setString(str);

    std::vector<mahjong::tile_t> tiles;
    for (size_t i = lastSuffixPos; i < len; ++i) {
        tiles.push_back(mahjong::make_tile(suit + 1, str[i] - '0'));
    }
    addTiles(&tiles.front(), tiles.size());
}

void TilesKeyboard::onHonor(int honor) {
    const std::string &originText = _textLabel->getString();
    if (!originText.empty() && isdigit(originText.back())) {
        return;
    }

    const char *honorFace = "ESWNCFP";
    std::string str = originText;
    str.append(1, honorFace[honor]);
    _textLabel->setString(str);

    mahjong::tile_t tile = mahjong::TILE_E + honor;
    addTiles(&tile, 1);
}

void TilesKeyboard::onBackspace() {
    const std::string &originText = _textLabel->getString();
    if (originText.empty()) {
        return;
    }

    std::string str = originText;
    char ch = str.back();
    str.pop_back();
    _textLabel->setString(str);

    switch (ch) {
    case 'm': case 's': case 'p': {
        std::string::size_type len = originText.length();
        std::string::size_type lastSuffixPos
            = std::find_if_not(originText.rbegin(), originText.rend(), isdigit).base() - originText.begin();

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
    const std::string &originText = _textLabel->getString();
    if (!originText.empty() && originText.back() == ' ') {
        return;
    }

    std::string str = _textLabel->getString();
    str.append(1, ' ');
    _textLabel->setString(str);

    addSpaceTile();
}

void TilesKeyboard::onLeftBracket() {
    if (_inBracket) {
        return;
    }

    const std::string &originText = _textLabel->getString();
    if (!originText.empty() && isdigit(originText.back())) {
        return;
    }

    _inBracket = true;
    std::string str = _textLabel->getString();
    str.append(1, '[');
    _textLabel->setString(str);

    addSpaceTile();
}

void TilesKeyboard::onRightBracket() {
    if (!_inBracket) {
        return;
    }

    const std::string &originText = _textLabel->getString();
    if (!originText.empty() && isdigit(originText.back())) {
        return;
    }

    _inBracket = false;
    std::string str = _textLabel->getString();
    str.append(1, ']');
    _textLabel->setString(str);

    addSpaceTile();
}
