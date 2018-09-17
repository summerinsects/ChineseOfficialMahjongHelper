#include "TilePickWidget.h"
#include "../utils/compiler.h"
#include "../UICommon.h"
#include "../UIColors.h"
#include "../TilesImage.h"

USING_NS_CC;

bool TilePickWidget::init(float maxWidth) {
    if (UNLIKELY(!Node::init())) {
        return false;
    }

    this->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    this->setIgnoreAnchorPointForPosition(false);

#define GAP 5

    // 下方的手牌
    HandTilesWidget *handTilesWidget = HandTilesWidget::create();
    this->addChild(handTilesWidget);
    Size handTilesSize = handTilesWidget->getContentSize();
    if (handTilesSize.width > maxWidth) {  // 缩放
        const float scale = maxWidth / handTilesSize.width;
        handTilesWidget->setScale(scale);
        handTilesSize.width = maxWidth;
        handTilesSize.height *= scale;
    }
    handTilesWidget->setPosition(Vec2(handTilesSize.width * 0.5f, handTilesSize.height * 0.5f));
    _handTilesWidget = handTilesWidget;

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

    // 适配
    if (tableSize.width + rightSize.width + 5 > maxWidth) {
        const Size tempSize = tableSize;

        // 缩放左边的选牌面板
        const float scale = (maxWidth - rightSize.width - 5) / tableSize.width;
        tilesContainer->setScale(scale);
        tableSize.width *= scale;
        tableSize.height *= scale;

        // 左边缩放后小于右边高度，按右边的高度重新适配
        if (tableSize.height < rightSize.height) {
            tableSize = tempSize;

            const float scale1 = rightSize.height / tableSize.height;
            tableSize.width *= scale1;
            tableSize.height *= scale1;

            const float scale2 = (maxWidth - 5) / (tableSize.width + rightSize.width);
            buttonsContainer->setScale(scale2);
            rightSize.width *= scale2;
            rightSize.height *= scale2;

            tilesContainer->setScale(scale1 * scale2);
            tableSize.width *= scale2;
            tableSize.height *= scale2;
        }
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
        mahjong::tile_t tile = mahjong::make_tile(TILE_SUIT_CHARACTERS, static_cast<mahjong::rank_t>(i + 1));
        ui::Button *button = ui::Button::create(tilesImageName[tile]);
        button->setScale(contentScaleFactor);
        tilesContainer->addChild(button);
        button->setPosition(Vec2(TILE_WIDTH * (i + 0.5f), charactersY));
        button->setTag(tile);
        button->addClickEventListener(std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1));
        _characterButtons[i] = button;
    }

    // 条
    const float bambooY = TILE_HEIGHT * 2.5f;
    for (int i = 0; i < 9; ++i) {
        mahjong::tile_t tile = mahjong::make_tile(TILE_SUIT_BAMBOO, static_cast<mahjong::rank_t>(i + 1));
        ui::Button *button = ui::Button::create(tilesImageName[tile]);
        button->setScale(contentScaleFactor);
        tilesContainer->addChild(button);
        button->setPosition(Vec2(TILE_WIDTH * (i + 0.5f), bambooY));
        button->setTag(tile);
        button->addClickEventListener(std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1));
        _bambooButtons[i] = button;
    }

    // 饼
    const float dotsY = TILE_HEIGHT * 1.5f;
    for (int i = 0; i < 9; ++i) {
        mahjong::tile_t tile = mahjong::make_tile(TILE_SUIT_DOTS, static_cast<mahjong::rank_t>(i + 1));
        ui::Button *button = ui::Button::create(tilesImageName[tile]);
        button->setScale(contentScaleFactor);
        tilesContainer->addChild(button);
        button->setPosition(Vec2(TILE_WIDTH * (i + 0.5f), dotsY));
        button->setTag(tile);
        button->addClickEventListener(std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1));
        _dotsButtons[i] = button;
    }

    // 字牌
    const float honorsY = TILE_HEIGHT * 0.5f;
    for (int i = 0; i < 7; ++i) {
        mahjong::tile_t tile = mahjong::make_tile(TILE_SUIT_HONORS, static_cast<mahjong::rank_t>(i + 1));
        ui::Button *button = ui::Button::create(tilesImageName[tile]);
        button->setScale(contentScaleFactor);
        tilesContainer->addChild(button);
        button->setPosition(Vec2(TILE_WIDTH * (i + 0.5f), honorsY));
        button->setTag(tile);
        button->addClickEventListener(std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1));
        _honorButtons[i] = button;
    }

    // 排列按钮
    ui::Button *buttons[8];
    static const char *titleText[8] = { __UTF8("吃 _XX"), __UTF8("吃 X_X"), __UTF8("吃 XX_"), __UTF8("排序"), __UTF8("碰"), __UTF8("明杠"), __UTF8("暗杠"), __UTF8("清空") };

    for (int i = 0; i < 8; ++i) {
        int col = i >> 2;
        int row = i & 3;

        ui::Button *button = UICommon::createButton();
        button->setScale9Enabled(true);
        button->setContentSize(Size(BUTTON_WIDHT, BUTTON_HEIGHT));
        button->setTitleFontSize(FONT_SIZE);
        button->setTitleColor(C3B_GRAY);
        button->setTitleText(titleText[i]);
        buttonsContainer->addChild(button);
        button->setPosition(Vec2(BUTTON_WIDHT * (col + 0.5f) + col * GAP, static_cast<float>((3 - row) * (BUTTON_HEIGHT + GAP_H) + BUTTON_HEIGHT / 2)));
        buttons[i] = button;
    }

    // 吃(_XX) 为23吃1这种类型
    // 吃(X_X) 为13吃2这种类型
    // 吃(XX_) 为12吃3这种类型
    for (int i = 0; i < 3; ++i) {
        buttons[i]->addClickEventListener([this, i](Ref *) { makeFixedSet(std::bind(&HandTilesWidget::makeFixedChowPack, _handTilesWidget, i)); });
        _chowButton[i] = buttons[i];
    }

    // 排序
    buttons[3]->addClickEventListener([this](Ref *) {  sort(); });

    // 碰
    buttons[4]->addClickEventListener([this](Ref *) { makeFixedSet(std::bind(&HandTilesWidget::makeFixedPungPack, _handTilesWidget)); });
    _pungButton = buttons[4];

    // 明杠
    buttons[5]->addClickEventListener([this](Ref *) { makeFixedSet(std::bind(&HandTilesWidget::makeFixedMeldedKongPack, _handTilesWidget)); });
    _meldedKongButton = buttons[5];

    // 暗杠
    buttons[6]->addClickEventListener([this](Ref *) { makeFixedSet(std::bind(&HandTilesWidget::makeFixedConcealedKongPack, _handTilesWidget)); });
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

void TilePickWidget::onTileTableButton(cocos2d::Ref *sender) {
    mahjong::tile_t tile = static_cast<mahjong::tile_t>(((ui::Button *)sender)->getTag());
    mahjong::tile_t prevTile = _handTilesWidget->putTile(tile);
    if (prevTile != 0 && prevTile != tile) {  // 如果是替换牌，则会删了一张旧的牌
        refreshTilesTableButton(prevTile);
    }
    refreshTilesTableButton(tile);
    if (LIKELY(_winTileChangedCallback)) {
        _winTileChangedCallback();
    }
}

void TilePickWidget::makeFixedSet(const std::function<bool ()> &makeFixedSetFunction) {
    mahjong::tile_t servingTile = _handTilesWidget->getServingTile();
    if (LIKELY(makeFixedSetFunction())) {
        if (LIKELY(_fixedPacksChangedCallback)) {
            _fixedPacksChangedCallback();
        }
        if (servingTile != _handTilesWidget->getServingTile() && LIKELY(_winTileChangedCallback)) {
            _winTileChangedCallback();
        }
    }
}
