#include "TilePickWidget.h"
#include "HandTilesWidget.h"
#include "../common.h"
#include "../compiler.h"

USING_NS_CC;

bool TilePickWidget::init() {
    if (UNLIKELY(!ui::Widget::init())) {
        return false;
    }

    _handTilesWidget = HandTilesWidget::create();
    this->addChild(_handTilesWidget);

    const Size &handTilesSize = _handTilesWidget->getContentSize();

    // 一张牌的尺寸：27 * 39
    // 27 * 9 = 243
    // 39 * 4 = 156
    Size tableSize = Size(243, 156);
    ui::Widget *tableWidget = ui::Widget::create();
    tableWidget->setContentSize(tableSize);
    this->addChild(tableWidget);
    const float tableBottom = handTilesSize.height + 4;
    tableWidget->setPosition(Vec2(tableSize.width * 0.5f + 10, tableBottom + tableSize.height * 0.5f));

    this->setContentSize(Size(handTilesSize.width, handTilesSize.height + tableSize.height + 4));
    _handTilesWidget->setPosition(Vec2(handTilesSize.width * 0.5f, handTilesSize.height * 0.5f));

    // 万
    for (int i = 0; i < 9; ++i) {
        mahjong::TILE tile = mahjong::make_tile(TILE_SUIT_CHARACTERS, i + 1);
        _characterButtons[i] = ui::Button::create(tilesImageName[tile]);
        _characterButtons[i]->setScale(27 / _characterButtons[i]->getContentSize().width);
        tableWidget->addChild(_characterButtons[i]);
        _characterButtons[i]->setPosition(Vec2(27 * (i + 0.5f), 136.5f));
        _characterButtons[i]->addClickEventListener(
            std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1, tile));
    }

    // 条
    for (int i = 0; i < 9; ++i) {
        mahjong::TILE tile = mahjong::make_tile(TILE_SUIT_BAMBOO, i + 1);
        _bambooButtons[i] = ui::Button::create(tilesImageName[tile]);
        _bambooButtons[i]->setScale(27 / _bambooButtons[i]->getContentSize().width);
        tableWidget->addChild(_bambooButtons[i]);
        _bambooButtons[i]->setPosition(Vec2(27 * (i + 0.5f), 97.5f));
        _bambooButtons[i]->addClickEventListener(
            std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1, tile));
    }

    // 饼
    for (int i = 0; i < 9; ++i) {
        mahjong::TILE tile = mahjong::make_tile(TILE_SUIT_DOTS, i + 1);
        _dotsButtons[i] = ui::Button::create(tilesImageName[tile]);
        _dotsButtons[i]->setScale(27 / _dotsButtons[i]->getContentSize().width);
        tableWidget->addChild(_dotsButtons[i]);
        _dotsButtons[i]->setPosition(Vec2(27 * (i + 0.5f), 58.5f));
        _dotsButtons[i]->addClickEventListener(
            std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1, tile));
    }

    // 风
    for (int i = 0; i < 4; ++i) {
        mahjong::TILE tile = mahjong::make_tile(TILE_SUIT_WINDS, i + 1);
        _honorButtons[i] = ui::Button::create(tilesImageName[tile]);
        _honorButtons[i]->setScale(27 / _honorButtons[i]->getContentSize().width);
        tableWidget->addChild(_honorButtons[i]);
        _honorButtons[i]->setPosition(Vec2(27 * (i + 0.5f), 19.5f));
        _honorButtons[i]->addClickEventListener(
            std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1, tile));
    }

    // 箭
    for (int i = 4; i < 7; ++i) {
        mahjong::TILE tile = mahjong::make_tile(TILE_SUIT_DRAGONS, i - 3);
        _honorButtons[i] = ui::Button::create(tilesImageName[tile]);
        _honorButtons[i]->setScale(27 / _honorButtons[i]->getContentSize().width);
        tableWidget->addChild(_honorButtons[i]);
        _honorButtons[i]->setPosition(Vec2(27 * (i + 0.5f), 19.5f));
        _honorButtons[i]->addClickEventListener(
            std::bind(&TilePickWidget::onTileTableButton, this, std::placeholders::_1, tile));
    }

    const char *normalImage, *selectedImage;
    if (UserDefault::getInstance()->getBoolForKey("night_mode")) {
        normalImage = "source_material/btn_square_normal.png";
        selectedImage = "source_material/btn_square_highlighted.png";
    }
    else {
        normalImage = "source_material/btn_square_highlighted.png";
        selectedImage = "source_material/btn_square_selected.png";
    }
    const char *disableImage = "source_material/btn_square_disabled.png";

    // 吃(_XX) 为23吃1这种类型
    _chow_XXButton = ui::Button::create(normalImage, selectedImage, disableImage);
    _chow_XXButton->setScale9Enabled(true);
    _chow_XXButton->setContentSize(Size(45.0f, 20.0f));
    _chow_XXButton->setTitleFontSize(12);
    _chow_XXButton->setTitleText("吃(_XX)");
    _chow_XXButton->setTitleColor(Color3B::BLACK);
    this->addChild(_chow_XXButton);
    _chow_XXButton->setPosition(Vec2(handTilesSize.width - 90, tableBottom + 140));
    _chow_XXButton->addClickEventListener(std::bind(&TilePickWidget::onChow_XXButton, this, std::placeholders::_1));

    // 吃(X_X) 为13吃2这种类型
    _chowX_XButton = ui::Button::create(normalImage, selectedImage, disableImage);
    _chowX_XButton->setScale9Enabled(true);
    _chowX_XButton->setContentSize(Size(45.0f, 20.0f));
    _chowX_XButton->setTitleFontSize(12);
    _chowX_XButton->setTitleText("吃(X_X)");
    _chowX_XButton->setTitleColor(Color3B::BLACK);
    this->addChild(_chowX_XButton);
    _chowX_XButton->setPosition(Vec2(handTilesSize.width - 90, tableBottom + 100));
    _chowX_XButton->addClickEventListener(std::bind(&TilePickWidget::onChowX_XButton, this, std::placeholders::_1));

    // 吃(XX_) 为12吃3这种类型
    _chowXX_Button = ui::Button::create(normalImage, selectedImage, disableImage);
    _chowXX_Button->setScale9Enabled(true);
    _chowXX_Button->setContentSize(Size(45.0f, 20.0f));
    _chowXX_Button->setTitleFontSize(12);
    _chowXX_Button->setTitleText("吃(XX_)");
    _chowXX_Button->setTitleColor(Color3B::BLACK);
    this->addChild(_chowXX_Button);
    _chowXX_Button->setPosition(Vec2(handTilesSize.width - 90, tableBottom + 60));
    _chowXX_Button->addClickEventListener(std::bind(&TilePickWidget::onChowXX_Button, this, std::placeholders::_1));

    // 碰
    _pungButton = ui::Button::create(normalImage, selectedImage, disableImage);
    _pungButton->setScale9Enabled(true);
    _pungButton->setContentSize(Size(45.0f, 20.0f));
    _pungButton->setTitleFontSize(12);
    _pungButton->setTitleText("碰");
    _pungButton->setTitleColor(Color3B::BLACK);
    this->addChild(_pungButton);
    _pungButton->setPosition(Vec2(handTilesSize.width - 40, tableBottom + 140));
    _pungButton->addClickEventListener(std::bind(&TilePickWidget::onPungButton, this, std::placeholders::_1));

    // 明杠
    _meldedKongButton = ui::Button::create(normalImage, selectedImage, disableImage);
    _meldedKongButton->setScale9Enabled(true);
    _meldedKongButton->setContentSize(Size(45.0f, 20.0f));
    _meldedKongButton->setTitleFontSize(12);
    _meldedKongButton->setTitleText("明杠");
    _meldedKongButton->setTitleColor(Color3B::BLACK);
    this->addChild(_meldedKongButton);
    _meldedKongButton->setPosition(Vec2(handTilesSize.width - 40, tableBottom + 100));
    _meldedKongButton->addClickEventListener(std::bind(&TilePickWidget::onMeldedKongButton, this, std::placeholders::_1));

    // 暗杠
    _concealedKongButton = ui::Button::create(normalImage, selectedImage, disableImage);
    _concealedKongButton->setScale9Enabled(true);
    _concealedKongButton->setContentSize(Size(45.0f, 20.0f));
    _concealedKongButton->setTitleFontSize(12);
    _concealedKongButton->setTitleText("暗杠");
    _concealedKongButton->setTitleColor(Color3B::BLACK);
    this->addChild(_concealedKongButton);
    _concealedKongButton->setPosition(Vec2(handTilesSize.width - 40, tableBottom + 60));
    _concealedKongButton->addClickEventListener(std::bind(&TilePickWidget::onConcealedKongButton, this, std::placeholders::_1));

    // 排序
    ui::Button *sortButton = ui::Button::create(normalImage, selectedImage, disableImage);
    sortButton->setScale9Enabled(true);
    sortButton->setContentSize(Size(45.0f, 20.0f));
    sortButton->setTitleFontSize(12);
    sortButton->setTitleText("排序");
    sortButton->setTitleColor(Color3B::BLACK);
    this->addChild(sortButton);
    sortButton->setPosition(Vec2(handTilesSize.width - 90, tableBottom + 20));
    sortButton->addClickEventListener([this](Ref *) {  _handTilesWidget->sort(); });

    // 重置
    ui::Button *clearButton = ui::Button::create(normalImage, selectedImage, disableImage);
    clearButton->setScale9Enabled(true);
    clearButton->setContentSize(Size(45.0f, 20.0f));
    clearButton->setTitleFontSize(12);
    clearButton->setTitleText("重置");
    clearButton->setTitleColor(Color3B::BLACK);
    this->addChild(clearButton);
    clearButton->setPosition(Vec2(handTilesSize.width - 40, tableBottom + 20));
    clearButton->addClickEventListener([this](Ref *) { reset(); });

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
    _chow_XXButton->setEnabled(false);
    _chowX_XButton->setEnabled(false);
    _chowXX_Button->setEnabled(false);
    _pungButton->setEnabled(false);
    _meldedKongButton->setEnabled(false);
    _concealedKongButton->setEnabled(false);

    if (LIKELY(_fixedSetsChangedCallback)) {
        _fixedSetsChangedCallback();
    }
    if (LIKELY(_winTileChangedCallback)) {
        _winTileChangedCallback();
    }
}

void TilePickWidget::sort() {
    _handTilesWidget->sort();

    if (LIKELY(_fixedSetsChangedCallback)) {
        _fixedSetsChangedCallback();
    }
    if (LIKELY(_winTileChangedCallback)) {
        _winTileChangedCallback();
    }
}

void TilePickWidget::setData(const mahjong::SET fixedSets[5], long setCnt, const mahjong::TILE standingTiles[13], long tileCnt, mahjong::TILE winTile) {
    _handTilesWidget->reset();

    _handTilesWidget->setData(fixedSets, setCnt, standingTiles, tileCnt, winTile);
    refreshAllTilesTableButton();
    refreshActionButtons();

    if (LIKELY(_fixedSetsChangedCallback)) {
        _fixedSetsChangedCallback();
    }
    if (LIKELY(_winTileChangedCallback)) {
        _winTileChangedCallback();
    }
}

// 刷新选牌按钮
void TilePickWidget::refreshTilesTableButton(mahjong::TILE tile) {
    // 如果某张牌已经使用了4张，那就禁用相应按钮
    int n = _handTilesWidget->getUsedTileCount(tile);
    mahjong::SUIT_TYPE suit = mahjong::tile_suit(tile);
    mahjong::RANK_TYPE rank = mahjong::tile_rank(tile);
    switch (suit) {
    case TILE_SUIT_CHARACTERS: _characterButtons[rank - 1]->setEnabled(n < 4); break;
    case TILE_SUIT_BAMBOO: _bambooButtons[rank - 1]->setEnabled(n < 4); break;
    case TILE_SUIT_DOTS: _dotsButtons[rank - 1]->setEnabled(n < 4); break;
    case TILE_SUIT_WINDS: _honorButtons[rank - 1]->setEnabled(n < 4); break;
    case TILE_SUIT_DRAGONS: _honorButtons[rank + 3]->setEnabled(n < 4); break;
    default: break;
    }
}

void TilePickWidget::refreshAllTilesTableButton() {
    for (mahjong::RANK_TYPE rank = 1; rank < 10; ++rank) {
        mahjong::TILE tile = mahjong::make_tile(TILE_SUIT_CHARACTERS, rank);
        int n = _handTilesWidget->getUsedTileCount(tile);
        _characterButtons[rank - 1]->setEnabled(n < 4);

        tile = mahjong::make_tile(TILE_SUIT_BAMBOO, rank);
        n = _handTilesWidget->getUsedTileCount(tile);
        _bambooButtons[rank - 1]->setEnabled(n < 4);

        tile = mahjong::make_tile(TILE_SUIT_DOTS, rank);
        n = _handTilesWidget->getUsedTileCount(tile);
        _dotsButtons[rank - 1]->setEnabled(n < 4);
    }

    for (mahjong::RANK_TYPE rank = 1; rank < 5; ++rank) {
        mahjong::TILE tile = mahjong::make_tile(TILE_SUIT_WINDS, rank);
        int n = _handTilesWidget->getUsedTileCount(tile);
        _honorButtons[rank - 1]->setEnabled(n < 4);
    }

    for (mahjong::RANK_TYPE rank = 1; rank < 4; ++rank) {
        mahjong::TILE tile = mahjong::make_tile(TILE_SUIT_DRAGONS, rank);
        int n = _handTilesWidget->getUsedTileCount(tile);
        _honorButtons[rank + 3]->setEnabled(n < 4);
    }
}

void TilePickWidget::refreshActionButtons() {
    _chow_XXButton->setEnabled(_handTilesWidget->canChow_XX());
    _chowX_XButton->setEnabled(_handTilesWidget->canChowX_X());
    _chowXX_Button->setEnabled(_handTilesWidget->canChowXX_());
    _pungButton->setEnabled(_handTilesWidget->canPung());
    _meldedKongButton->setEnabled(_handTilesWidget->canKong());
    _concealedKongButton->setEnabled(_meldedKongButton->isEnabled());
}

void TilePickWidget::onTileTableButton(cocos2d::Ref *sender, mahjong::TILE tile) {
    mahjong::TILE prevTile = _handTilesWidget->putTile(tile);
    if (prevTile != 0) {
        refreshTilesTableButton(prevTile);
    }
    refreshTilesTableButton(tile);
    if (LIKELY(_winTileChangedCallback)) {
        _winTileChangedCallback();
    }
}

void TilePickWidget::onChow_XXButton(cocos2d::Ref *sender) {
    if (LIKELY(_handTilesWidget->makeFixedChow_XXSet())) {
        if (LIKELY(_fixedSetsChangedCallback)) {
            _fixedSetsChangedCallback();
        }
    }
}

void TilePickWidget::onChowX_XButton(cocos2d::Ref *sender) {
    if (LIKELY(_handTilesWidget->makeFixedChowX_XSet())) {
        if (LIKELY(_fixedSetsChangedCallback)) {
            _fixedSetsChangedCallback();
        }
    }
}

void TilePickWidget::onChowXX_Button(cocos2d::Ref *sender) {
    if (LIKELY(_handTilesWidget->makeFixedChowXX_Set())) {
        if (LIKELY(_fixedSetsChangedCallback)) {
            _fixedSetsChangedCallback();
        }
    }
}

void TilePickWidget::onPungButton(cocos2d::Ref *sender) {
    if (LIKELY(_handTilesWidget->makeFixedPungSet())) {
        if (LIKELY(_fixedSetsChangedCallback)) {
            _fixedSetsChangedCallback();
        }
    }
}

void TilePickWidget::onMeldedKongButton(cocos2d::Ref *sender) {
    if (LIKELY(_handTilesWidget->makeFixedMeldedKongSet())) {
        if (LIKELY(_fixedSetsChangedCallback)) {
            _fixedSetsChangedCallback();
        }
    }
}

void TilePickWidget::onConcealedKongButton(cocos2d::Ref *sender) {
    _handTilesWidget->makeFixedConcealedKongSet();

    if (_fixedSetsChangedCallback) {
        _fixedSetsChangedCallback();
    }
}
