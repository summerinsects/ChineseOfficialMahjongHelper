#include "TilePickWidget.h"
#include "HandTilesWidget.h"
#include "../compiler.h"
#include "../TilesImage.h"

USING_NS_CC;

bool TilePickWidget::init() {
    if (UNLIKELY(!HandTilesWidget::init())) {
        return false;
    }

    const Size &handTilesSize = this->getContentSize();

    Size tableSize = Size(TILE_WIDTH * 9, TILE_HEIGHT * 4);
    ui::Widget *tableWidget = ui::Widget::create();
    tableWidget->setContentSize(tableSize);
    this->addChild(tableWidget);
    const float tableBottom = handTilesSize.height + 4;
    tableWidget->setPosition(Vec2(tableSize.width * 0.5f + 10, tableBottom + tableSize.height * 0.5f));

    this->setContentSize(Size(handTilesSize.width, handTilesSize.height + tableSize.height + 4));

    const float contentScaleFactor = CC_CONTENT_SCALE_FACTOR();

    // 万
    for (int i = 0; i < 9; ++i) {
        mahjong::tile_t tile = mahjong::make_tile(TILE_SUIT_CHARACTERS, i + 1);
        ui::Button *button = ui::Button::create(tilesImageName[tile]);
        button->setScale(contentScaleFactor);
        tableWidget->addChild(button);
        button->setPosition(Vec2(TILE_WIDTH * (i + 0.5f), TILE_HEIGHT * 3.5f));
        button->addClickEventListener(std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1, tile));
        _characterButtons[i] = button;
    }

    // 条
    for (int i = 0; i < 9; ++i) {
        mahjong::tile_t tile = mahjong::make_tile(TILE_SUIT_BAMBOO, i + 1);
        ui::Button *button = ui::Button::create(tilesImageName[tile]);
        button->setScale(contentScaleFactor);
        tableWidget->addChild(button);
        button->setPosition(Vec2(TILE_WIDTH * (i + 0.5f), TILE_HEIGHT * 2.5f));
        button->addClickEventListener(std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1, tile));
        _bambooButtons[i] = button;
    }

    // 饼
    for (int i = 0; i < 9; ++i) {
        mahjong::tile_t tile = mahjong::make_tile(TILE_SUIT_DOTS, i + 1);
        ui::Button *button = ui::Button::create(tilesImageName[tile]);
        button->setScale(contentScaleFactor);
        tableWidget->addChild(button);
        button->setPosition(Vec2(TILE_WIDTH * (i + 0.5f), TILE_HEIGHT * 1.5f));
        button->addClickEventListener(std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1, tile));
        _dotsButtons[i] = button;
    }

    // 字牌
    for (int i = 0; i < 7; ++i) {
        mahjong::tile_t tile = mahjong::make_tile(TILE_SUIT_HONORS, i + 1);
        ui::Button *button = ui::Button::create(tilesImageName[tile]);
        button->setScale(contentScaleFactor);
        tableWidget->addChild(button);
        button->setPosition(Vec2(TILE_WIDTH * (i + 0.5f), TILE_HEIGHT * 0.5f));
        button->addClickEventListener(std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1, tile));
        _honorButtons[i] = button;
    }

    const char *normalImage = "source_material/btn_square_highlighted.png";
    const char *selectedImage = "source_material/btn_square_selected.png";
    const char *disableImage = "source_material/btn_square_disabled.png";

    // 吃(_XX) 为23吃1这种类型
    ui::Button *button = ui::Button::create(normalImage, selectedImage, disableImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(45.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleColor(Color3B::BLACK);
    button->setTitleText("吃(_XX)");
    this->addChild(button);
    button->setPosition(Vec2(handTilesSize.width - 90, tableBottom + 140));
    button->addClickEventListener(std::bind(&TilePickWidget::onChow_XXButton, this, std::placeholders::_1));
    _chow_XXButton = button;

    // 吃(X_X) 为13吃2这种类型
    button = ui::Button::create(normalImage, selectedImage, disableImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(45.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleColor(Color3B::BLACK);
    button->setTitleText("吃(X_X)");
    this->addChild(button);
    button->setPosition(Vec2(handTilesSize.width - 90, tableBottom + 100));
    button->addClickEventListener(std::bind(&TilePickWidget::onChowX_XButton, this, std::placeholders::_1));
    _chowX_XButton = button;

    // 吃(XX_) 为12吃3这种类型
    button = ui::Button::create(normalImage, selectedImage, disableImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(45.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleColor(Color3B::BLACK);
    button->setTitleText("吃(XX_)");
    this->addChild(button);
    button->setPosition(Vec2(handTilesSize.width - 90, tableBottom + 60));
    button->addClickEventListener(std::bind(&TilePickWidget::onChowXX_Button, this, std::placeholders::_1));
    _chowXX_Button = button;

    // 碰
    button = ui::Button::create(normalImage, selectedImage, disableImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(45.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleColor(Color3B::BLACK);
    button->setTitleText("碰");
    this->addChild(button);
    button->setPosition(Vec2(handTilesSize.width - 40, tableBottom + 140));
    button->addClickEventListener(std::bind(&TilePickWidget::onPungButton, this, std::placeholders::_1));
    _pungButton = button;

    // 明杠
    button = ui::Button::create(normalImage, selectedImage, disableImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(45.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleColor(Color3B::BLACK);
    button->setTitleText("明杠");
    this->addChild(button);
    button->setPosition(Vec2(handTilesSize.width - 40, tableBottom + 100));
    button->addClickEventListener(std::bind(&TilePickWidget::onMeldedKongButton, this, std::placeholders::_1));
    _meldedKongButton = button;

    // 暗杠
    button = ui::Button::create(normalImage, selectedImage, disableImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(45.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleColor(Color3B::BLACK);
    button->setTitleText("暗杠");
    this->addChild(button);
    button->setPosition(Vec2(handTilesSize.width - 40, tableBottom + 60));
    button->addClickEventListener(std::bind(&TilePickWidget::onConcealedKongButton, this, std::placeholders::_1));
    _concealedKongButton = button;

    // 排序
    button = ui::Button::create(normalImage, selectedImage, disableImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(45.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleColor(Color3B::BLACK);
    button->setTitleText("排序");
    this->addChild(button);
    button->setPosition(Vec2(handTilesSize.width - 90, tableBottom + 20));
    button->addClickEventListener([this](Ref *) {  sort(); });

    // 重置
    button = ui::Button::create(normalImage, selectedImage, disableImage);
    button->setScale9Enabled(true);
    button->setContentSize(Size(45.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleColor(Color3B::BLACK);
    button->setTitleText("重置");
    this->addChild(button);
    button->setPosition(Vec2(handTilesSize.width - 40, tableBottom + 20));
    button->addClickEventListener([this](Ref *) { reset(); });

    reset();

    _currentIdxChangedCallback = std::bind(&TilePickWidget::refreshActionButtons, this);

    return true;
}

void TilePickWidget::reset() {
    HandTilesWidget::reset();

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
    _chow_XXButton->setEnabled(false);
    _chowX_XButton->setEnabled(false);
    _chowXX_Button->setEnabled(false);
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
    this->sortStandingTiles();

    if (LIKELY(_fixedPacksChangedCallback)) {
        _fixedPacksChangedCallback();
    }
    if (LIKELY(_winTileChangedCallback)) {
        _winTileChangedCallback();
    }
}

void TilePickWidget::setData(const mahjong::hand_tiles_t &hand_tiles, mahjong::tile_t winTile) {
    HandTilesWidget::setData(hand_tiles, winTile);
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
    int n = this->getUsedTileCount(tile);
    mahjong::suit_t suit = mahjong::tile_suit(tile);
    mahjong::rank_t rank = mahjong::tile_rank(tile);
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
        int n = this->getUsedTileCount(tile);
        _characterButtons[rank - 1]->setEnabled(n < 4);

        tile = mahjong::make_tile(TILE_SUIT_BAMBOO, rank);
        n = this->getUsedTileCount(tile);
        _bambooButtons[rank - 1]->setEnabled(n < 4);

        tile = mahjong::make_tile(TILE_SUIT_DOTS, rank);
        n = this->getUsedTileCount(tile);
        _dotsButtons[rank - 1]->setEnabled(n < 4);
    }

    // 字牌7种
    for (mahjong::rank_t rank = 1; rank < 8; ++rank) {
        mahjong::tile_t tile = mahjong::make_tile(TILE_SUIT_HONORS, rank);
        int n = this->getUsedTileCount(tile);
        _honorButtons[rank - 1]->setEnabled(n < 4);
    }
}

void TilePickWidget::refreshActionButtons() {
    _chow_XXButton->setEnabled(this->canChow_XX());
    _chowX_XButton->setEnabled(this->canChowX_X());
    _chowXX_Button->setEnabled(this->canChowXX_());
    _pungButton->setEnabled(this->canPung());
    _meldedKongButton->setEnabled(this->canKong());
    _concealedKongButton->setEnabled(_meldedKongButton->isEnabled());
}

void TilePickWidget::onTileTableButton(cocos2d::Ref *sender, mahjong::tile_t tile) {
    mahjong::tile_t prevTile = this->putTile(tile);
    if (prevTile != 0 && prevTile != tile) {  // 如果是替换牌，则会删了一张旧的牌
        refreshTilesTableButton(prevTile);
    }
    refreshTilesTableButton(tile);
    if (LIKELY(_winTileChangedCallback)) {
        _winTileChangedCallback();
    }
}

void TilePickWidget::onChow_XXButton(cocos2d::Ref *sender) {
    if (LIKELY(this->makeFixedChow_XXPack())) {
        if (LIKELY(_fixedPacksChangedCallback)) {
            _fixedPacksChangedCallback();
        }
    }
}

void TilePickWidget::onChowX_XButton(cocos2d::Ref *sender) {
    if (LIKELY(this->makeFixedChowX_XPack())) {
        if (LIKELY(_fixedPacksChangedCallback)) {
            _fixedPacksChangedCallback();
        }
    }
}

void TilePickWidget::onChowXX_Button(cocos2d::Ref *sender) {
    if (LIKELY(this->makeFixedChowXX_Pack())) {
        if (LIKELY(_fixedPacksChangedCallback)) {
            _fixedPacksChangedCallback();
        }
    }
}

void TilePickWidget::onPungButton(cocos2d::Ref *sender) {
    if (LIKELY(this->makeFixedPungPack())) {
        if (LIKELY(_fixedPacksChangedCallback)) {
            _fixedPacksChangedCallback();
        }
    }
}

void TilePickWidget::onMeldedKongButton(cocos2d::Ref *sender) {
    if (LIKELY(this->makeFixedMeldedKongPack())) {
        if (LIKELY(_fixedPacksChangedCallback)) {
            _fixedPacksChangedCallback();
        }
    }
}

void TilePickWidget::onConcealedKongButton(cocos2d::Ref *sender) {
    if (LIKELY(this->makeFixedConcealedKongPack())) {
        if (LIKELY(_fixedPacksChangedCallback)) {
            _fixedPacksChangedCallback();
        }
    }
}
