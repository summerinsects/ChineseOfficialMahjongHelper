#include "TilePickWidget.h"
#include "../utils/compiler.h"
#include "../TilesImage.h"

USING_NS_CC;

bool TilePickWidget::init() {
    if (UNLIKELY(!Node::init())) {
        return false;
    }

    this->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    this->setIgnoreAnchorPointForPosition(false);

#define GAP 5

    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float maxWidth = visibleSize.width - GAP * 2;

    // 下方的手牌
    _handTilesWidget = HandTilesWidget::create();
    this->addChild(_handTilesWidget);
    Size handTilesSize = _handTilesWidget->getContentSize();
    if (handTilesSize.width > maxWidth) {  // 缩放
        const float scale = maxWidth / handTilesSize.width;
        _handTilesWidget->setScale(scale);
        handTilesSize.width = maxWidth;
        handTilesSize.height *= scale;
    }
    _handTilesWidget->setPosition(Vec2(handTilesSize.width * 0.5f, handTilesSize.height * 0.5f));

    // 上方左边的牌面板
    Size tableSize = Size(TILE_WIDTH * 9, TILE_HEIGHT * 4);
    Node *tilesContainer = Node::create();
    tilesContainer->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    tilesContainer->setIgnoreAnchorPointForPosition(false);
    tilesContainer->setContentSize(tableSize);
    this->addChild(tilesContainer);

#define BUTTON_WIDHT 40
#define BUTTON_HEIGHT 20
#define FONT_SIZE 12
#define GAP_H (GAP * 2)

    // 上方右边的按钮
    Size rightSize = Size(BUTTON_WIDHT * 2 + GAP, BUTTON_HEIGHT * 4 + GAP_H * 3);
    Node *buttonsContainer = Node::create();
    buttonsContainer->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    buttonsContainer->setIgnoreAnchorPointForPosition(false);
    buttonsContainer->setContentSize(rightSize);
    this->addChild(buttonsContainer);

    if (tableSize.width > maxWidth - rightSize.width - 5) {  // 缩放左边的选牌面板
        const float scale = (maxWidth - rightSize.width - 5) / tableSize.width;
        tilesContainer->setScale(scale);
        tableSize.width *= scale;
        tableSize.height *= scale;
    }

    const float maxHeight = std::max(rightSize.height, tableSize.height);
    tilesContainer->setPosition(Vec2(tableSize.width * 0.5f, handTilesSize.height + 5 + maxHeight * 0.5f));
    buttonsContainer->setPosition(Vec2(maxWidth - rightSize.width * 0.5f, handTilesSize.height + 5 + maxHeight * 0.5f));
    this->setContentSize(Size(maxWidth, handTilesSize.height + maxHeight + 5));

    // 排列牌
    const float contentScaleFactor = CC_CONTENT_SCALE_FACTOR();

    // 万
    const float charactersY = TILE_HEIGHT * 3.5f;
    for (int i = 0; i < 9; ++i) {
        mahjong::tile_t tile = mahjong::make_tile(TILE_SUIT_CHARACTERS, i + 1);
        ui::Button *button = ui::Button::create(tilesImageName[tile]);
        button->setScale(contentScaleFactor);
        tilesContainer->addChild(button);
        button->setPosition(Vec2(TILE_WIDTH * (i + 0.5f), charactersY));
        button->addClickEventListener(std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1, tile));
        _characterButtons[i] = button;
    }

    // 条
    const float bambooY = TILE_HEIGHT * 2.5f;
    for (int i = 0; i < 9; ++i) {
        mahjong::tile_t tile = mahjong::make_tile(TILE_SUIT_BAMBOO, i + 1);
        ui::Button *button = ui::Button::create(tilesImageName[tile]);
        button->setScale(contentScaleFactor);
        tilesContainer->addChild(button);
        button->setPosition(Vec2(TILE_WIDTH * (i + 0.5f), bambooY));
        button->addClickEventListener(std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1, tile));
        _bambooButtons[i] = button;
    }

    // 饼
    const float dotsY = TILE_HEIGHT * 1.5f;
    for (int i = 0; i < 9; ++i) {
        mahjong::tile_t tile = mahjong::make_tile(TILE_SUIT_DOTS, i + 1);
        ui::Button *button = ui::Button::create(tilesImageName[tile]);
        button->setScale(contentScaleFactor);
        tilesContainer->addChild(button);
        button->setPosition(Vec2(TILE_WIDTH * (i + 0.5f), dotsY));
        button->addClickEventListener(std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1, tile));
        _dotsButtons[i] = button;
    }

    // 字牌
    const float honorsY = TILE_HEIGHT * 0.5f;
    for (int i = 0; i < 7; ++i) {
        mahjong::tile_t tile = mahjong::make_tile(TILE_SUIT_HONORS, i + 1);
        ui::Button *button = ui::Button::create(tilesImageName[tile]);
        button->setScale(contentScaleFactor);
        tilesContainer->addChild(button);
        button->setPosition(Vec2(TILE_WIDTH * (i + 0.5f), honorsY));
        button->addClickEventListener(std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1, tile));
        _honorButtons[i] = button;
    }

    // 排列按钮
    ui::Button *buttons[8];
    static const char *titleText[8] = {"吃 _XX", "吃 X_X", "吃 XX_", "排序", "碰", "明杠", "暗杠", "清空" };

    for (int i = 0; i < 8; ++i) {
        int col = i >> 2;
        int row = i & 3;

        ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
        button->setScale9Enabled(true);
        button->setContentSize(Size(BUTTON_WIDHT, BUTTON_HEIGHT));
        button->setTitleFontSize(FONT_SIZE);
        button->setTitleColor(Color3B::BLACK);
        button->setTitleText(titleText[i]);
        buttonsContainer->addChild(button);
        button->setPosition(Vec2(BUTTON_WIDHT * (col + 0.5f) + col * GAP, static_cast<float>((3 - row) * (BUTTON_HEIGHT + GAP_H) + BUTTON_HEIGHT / 2)));
        buttons[i] = button;
    }

    // 吃(_XX) 为23吃1这种类型
    // 吃(X_X) 为13吃2这种类型
    // 吃(XX_) 为12吃3这种类型
    for (int i = 0; i < 3; ++i) {
        buttons[i]->addClickEventListener(std::bind(&TilePickWidget::onChowButton, this, std::placeholders::_1));
        _chowButton[i] = buttons[i];
        _chowButton[i]->setTag(i);
    }

    // 排序
    buttons[3]->addClickEventListener([this](Ref *) {  sort(); });

    // 碰
    buttons[4]->addClickEventListener(std::bind(&TilePickWidget::onPungButton, this, std::placeholders::_1));
    _pungButton = buttons[4];

    // 明杠
    buttons[5]->addClickEventListener(std::bind(&TilePickWidget::onMeldedKongButton, this, std::placeholders::_1));
    _meldedKongButton = buttons[5];

    // 暗杠
    buttons[6]->addClickEventListener(std::bind(&TilePickWidget::onConcealedKongButton, this, std::placeholders::_1));
    _concealedKongButton = buttons[6];

    // 重置
    buttons[7]->addClickEventListener([this](Ref *) { reset(); });

    reset();

    _handTilesWidget->setCurrentIdxChangedCallback(std::bind(&TilePickWidget::refreshActionButtons, this));

    return true;
}

void TilePickWidget::reset() {
    _handTilesWidget->reset();

    // 所有牌按钮都启用
    for (int i = 0; i < 9; ++i) {
        _characterButtons[i]->setEnabled(true);
        _bambooButtons[i]->setEnabled(true);
        _dotsButtons[i]->setEnabled(true);
    }
    for (int i = 0; i < 7; ++i) {
        _honorButtons[i]->setEnabled(true);
    }

    // 禁用吃碰杠按钮
    for (int i = 0; i < 3; ++i) {
        _chowButton[i]->setEnabled(false);
    }
    _pungButton->setEnabled(false);
    _meldedKongButton->setEnabled(false);
    _concealedKongButton->setEnabled(false);

    if (LIKELY(_fixedPacksChangedCallback)) {
        _fixedPacksChangedCallback();
    }
    if (LIKELY(_winTileChangedCallback)) {
        _winTileChangedCallback();
    }
}

void TilePickWidget::sort() {
    _handTilesWidget->sortStandingTiles();

    if (LIKELY(_fixedPacksChangedCallback)) {
        _fixedPacksChangedCallback();
    }
    if (LIKELY(_winTileChangedCallback)) {
        _winTileChangedCallback();
    }
}

void TilePickWidget::setData(const mahjong::hand_tiles_t &hand_tiles, mahjong::tile_t winTile) {
    _handTilesWidget->setData(hand_tiles, winTile);
    refreshAllTilesTableButton();
    refreshActionButtons();

    if (LIKELY(_fixedPacksChangedCallback)) {
        _fixedPacksChangedCallback();
    }
    if (LIKELY(_winTileChangedCallback)) {
        _winTileChangedCallback();
    }
}

// 刷新选牌按钮
void TilePickWidget::refreshTilesTableButton(mahjong::tile_t tile) {
    // 如果某张牌已经使用了4张，就禁用相应按钮
    int n = _handTilesWidget->getUsedTileCount(tile);
    mahjong::suit_t suit = mahjong::tile_get_suit(tile);
    mahjong::rank_t rank = mahjong::tile_get_rank(tile);
    switch (suit) {
    case TILE_SUIT_CHARACTERS: _characterButtons[rank - 1]->setEnabled(n < 4); break;
    case TILE_SUIT_BAMBOO: _bambooButtons[rank - 1]->setEnabled(n < 4); break;
    case TILE_SUIT_DOTS: _dotsButtons[rank - 1]->setEnabled(n < 4); break;
    case TILE_SUIT_HONORS: _honorButtons[rank - 1]->setEnabled(n < 4); break;
    default: break;
    }
}

void TilePickWidget::refreshAllTilesTableButton() {
    // 如果某张牌已经使用了4张，就禁用相应按钮
    // 数牌都是1-9，放在同一个循环里
    for (mahjong::rank_t rank = 1; rank < 10; ++rank) {
        mahjong::tile_t tile = mahjong::make_tile(TILE_SUIT_CHARACTERS, rank);
        int n = _handTilesWidget->getUsedTileCount(tile);
        _characterButtons[rank - 1]->setEnabled(n < 4);

        tile = mahjong::make_tile(TILE_SUIT_BAMBOO, rank);
        n = _handTilesWidget->getUsedTileCount(tile);
        _bambooButtons[rank - 1]->setEnabled(n < 4);

        tile = mahjong::make_tile(TILE_SUIT_DOTS, rank);
        n = _handTilesWidget->getUsedTileCount(tile);
        _dotsButtons[rank - 1]->setEnabled(n < 4);
    }

    // 字牌7种
    for (mahjong::rank_t rank = 1; rank < 8; ++rank) {
        mahjong::tile_t tile = mahjong::make_tile(TILE_SUIT_HONORS, rank);
        int n = _handTilesWidget->getUsedTileCount(tile);
        _honorButtons[rank - 1]->setEnabled(n < 4);
    }
}

void TilePickWidget::refreshActionButtons() {
    for (int i = 0; i < 3; ++i) {
        _chowButton[i]->setEnabled(_handTilesWidget->canChow(i));
    }
    _pungButton->setEnabled(_handTilesWidget->canPung());
    _meldedKongButton->setEnabled(_handTilesWidget->canKong());
    _concealedKongButton->setEnabled(_meldedKongButton->isEnabled());
}

void TilePickWidget::onTileTableButton(cocos2d::Ref *, mahjong::tile_t tile) {
    mahjong::tile_t prevTile = _handTilesWidget->putTile(tile);
    if (prevTile != 0 && prevTile != tile) {  // 如果是替换牌，则会删了一张旧的牌
        refreshTilesTableButton(prevTile);
    }
    refreshTilesTableButton(tile);
    if (LIKELY(_winTileChangedCallback)) {
        _winTileChangedCallback();
    }
}

void TilePickWidget::onChowButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    int pos = button->getTag();
    if (LIKELY(_handTilesWidget->makeFixedChowPack(pos))) {
        if (LIKELY(_fixedPacksChangedCallback)) {
            _fixedPacksChangedCallback();
        }
    }
}

void TilePickWidget::onPungButton(cocos2d::Ref *) {
    if (LIKELY(_handTilesWidget->makeFixedPungPack())) {
        if (LIKELY(_fixedPacksChangedCallback)) {
            _fixedPacksChangedCallback();
        }
    }
}

void TilePickWidget::onMeldedKongButton(cocos2d::Ref *) {
    if (LIKELY(_handTilesWidget->makeFixedMeldedKongPack())) {
        if (LIKELY(_fixedPacksChangedCallback)) {
            _fixedPacksChangedCallback();
        }
    }
}

void TilePickWidget::onConcealedKongButton(cocos2d::Ref *) {
    if (LIKELY(_handTilesWidget->makeFixedConcealedKongPack())) {
        if (LIKELY(_fixedPacksChangedCallback)) {
            _fixedPacksChangedCallback();
        }
    }
}
