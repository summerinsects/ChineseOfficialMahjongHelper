#include "TilePickWidget.h"
#include "../mahjong-algorithm/points_calculator.h"

USING_NS_CC;

static const char *imageName[] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "tiles/m1.png", "tiles/m2.png", "tiles/m3.png", "tiles/m4.png", "tiles/m5.png", "tiles/m6.png", "tiles/m7.png", "tiles/m8.png", "tiles/m9.png", "", "", "", "", "", "",
    "", "tiles/s1.png", "tiles/s2.png", "tiles/s3.png", "tiles/s4.png", "tiles/s5.png", "tiles/s6.png", "tiles/s7.png", "tiles/s8.png", "tiles/s9.png", "", "", "", "", "", "",
    "", "tiles/p1.png", "tiles/p2.png", "tiles/p3.png", "tiles/p4.png", "tiles/p5.png", "tiles/p6.png", "tiles/p7.png", "tiles/p8.png", "tiles/p9.png", "", "", "", "", "", "",
    "", "tiles/w1.png", "tiles/w2.png", "tiles/w3.png", "tiles/w4.png", "", "", "", "", "", "", "", "", "", "", "",
    "", "tiles/d1.png", "tiles/d2.png", "tiles/d3.png", "", "", "", "", "", "", "", "", "", "", "", "",
};

bool TilePickWidget::init() {
    if (!ui::Widget::init()) {
        return false;
    }

    // 一张牌的尺寸：27 * 39

    // 14张牌宽度：27 * 14 = 378
    // 加间距：378 + 4 = 382
    Size tilesSize = Size(382, 39);
    _tilesWidget = ui::Widget::create();
    _tilesWidget->setContentSize(tilesSize);
    this->addChild(_tilesWidget);
    _tilesWidget->setPosition(Vec2(tilesSize.width * 0.5f, tilesSize.height * 0.5f));

    _highlightBox = DrawNode::create();
    _highlightBox->setContentSize(Size(27, 39));
    _highlightBox->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    _highlightBox->drawLine(Vec2(0, 0), Vec2(27, 0), Color4F::RED);
    _highlightBox->drawLine(Vec2(27, 0), Vec2(27, 39), Color4F::RED);
    _highlightBox->drawLine(Vec2(27, 39), Vec2(0, 39), Color4F::RED);
    _highlightBox->drawLine(Vec2(0, 39), Vec2(0, 0), Color4F::RED);
    _tilesWidget->addChild(_highlightBox, 2);
    _highlightBox->setPosition(Vec2(27 * 0.5f, 39 * 0.5f));

    // 一个直杠的宽度：39 + 27 * 3 = 120
    // 两个直杠的宽度：120 * 2 = 240
    // 加间距：240 + 4 = 244

    // 高度：39 * 2 = 78
    // 加间距：78 + 4 = 82
    Size fixedSize = Size(244, 82);
    _fixedWidget = ui::Widget::create();
    _fixedWidget->setContentSize(fixedSize);
    this->addChild(_fixedWidget);
    _fixedWidget->setPosition(Vec2(tilesSize.width * 0.5f, tilesSize.height + fixedSize.height * 0.5f + 4));

    // 27 * 9 = 243
    // 39 * 4 = 156
    Size tableSize = Size(243, 156);
    ui::Widget *tableWidget = ui::Widget::create();
    tableWidget->setContentSize(tableSize);
    this->addChild(tableWidget);
    const float tableBottom = tilesSize.height + fixedSize.height + 8;
    tableWidget->setPosition(Vec2(tableSize.width * 0.5f + 10, tableBottom + tableSize.height * 0.5f));

    this->setContentSize(Size(tilesSize.width, tilesSize.height + fixedSize.height + tableSize.height + 8));

    for (int i = 0; i < 9; ++i) {
        TILE tile = make_tile(TILE_SUIT_CHARACTERS, i + 1);
        _characterButtons[i] = ui::Button::create(imageName[tile]);
        tableWidget->addChild(_characterButtons[i]);
        _characterButtons[i]->setPosition(Vec2(27 * (i + 0.5f), 136.5f));
        _characterButtons[i]->addClickEventListener(
            std::bind(&TilePickWidget::tileTableCallback, this, std::placeholders::_1, tile));
    }

    for (int i = 0; i < 9; ++i) {
        TILE tile = make_tile(TILE_SUIT_BAMBOO, i + 1);
        _bambooButtons[i] = ui::Button::create(imageName[tile]);
        tableWidget->addChild(_bambooButtons[i]);
        _bambooButtons[i]->setPosition(Vec2(27 * (i + 0.5f), 97.5f));
        _bambooButtons[i]->addClickEventListener(
            std::bind(&TilePickWidget::tileTableCallback, this, std::placeholders::_1, tile));
    }

    for (int i = 0; i < 9; ++i) {
        TILE tile = make_tile(TILE_SUIT_DOTS, i + 1);
        _dotsButtons[i] = ui::Button::create(imageName[tile]);
        tableWidget->addChild(_dotsButtons[i]);
        _dotsButtons[i]->setPosition(Vec2(27 * (i + 0.5f), 58.5f));
        _dotsButtons[i]->addClickEventListener(
            std::bind(&TilePickWidget::tileTableCallback, this, std::placeholders::_1, tile));
    }

    for (int i = 0; i < 4; ++i) {
        TILE tile = make_tile(TILE_SUIT_WINDS, i + 1);
        _honorButtons[i] = ui::Button::create(imageName[tile]);
        tableWidget->addChild(_honorButtons[i]);
        _honorButtons[i]->setPosition(Vec2(27 * (i + 0.5f), 19.5f));
        _honorButtons[i]->addClickEventListener(
            std::bind(&TilePickWidget::tileTableCallback, this, std::placeholders::_1, tile));
    }

    for (int i = 4; i < 7; ++i) {
        TILE tile = make_tile(TILE_SUIT_DRAGONS, i - 3);
        _honorButtons[i] = ui::Button::create(imageName[tile]);
        tableWidget->addChild(_honorButtons[i]);
        _honorButtons[i]->setPosition(Vec2(27 * (i + 0.5f), 19.5f));
        _honorButtons[i]->addClickEventListener(
            std::bind(&TilePickWidget::tileTableCallback, this, std::placeholders::_1, tile));
    }

    _chowLessButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
    _chowLessButton->setScale9Enabled(true);
    _chowLessButton->setContentSize(Size(40.0f, 25.0f));
    _chowLessButton->setTitleText("吃(XX_)");
    _chowLessButton->setTitleColor(Color3B::BLACK);
    this->addChild(_chowLessButton);
    _chowLessButton->setPosition(Vec2(tilesSize.width - 90, tableBottom + 140));
    _chowLessButton->addClickEventListener(std::bind(&TilePickWidget::chowLessCallback, this, std::placeholders::_1));

    _chowMidButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
    _chowMidButton->setScale9Enabled(true);
    _chowMidButton->setContentSize(Size(40.0f, 25.0f));
    _chowMidButton->setTitleText("吃(X_X)");
    _chowMidButton->setTitleColor(Color3B::BLACK);
    this->addChild(_chowMidButton);
    _chowMidButton->setPosition(Vec2(tilesSize.width - 90, tableBottom + 100));
    _chowMidButton->addClickEventListener(std::bind(&TilePickWidget::chowMidCallback, this, std::placeholders::_1));

    _chowGreatButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
    _chowGreatButton->setScale9Enabled(true);
    _chowGreatButton->setContentSize(Size(40.0f, 25.0f));
    _chowGreatButton->setTitleText("吃(_XX)");
    _chowGreatButton->setTitleColor(Color3B::BLACK);
    this->addChild(_chowGreatButton);
    _chowGreatButton->setPosition(Vec2(tilesSize.width - 90, tableBottom + 60));
    _chowGreatButton->addClickEventListener(std::bind(&TilePickWidget::chowGreatCallback, this, std::placeholders::_1));

    _pungButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
    _pungButton->setScale9Enabled(true);
    _pungButton->setContentSize(Size(40.0f, 25.0f));
    _pungButton->setTitleText("碰");
    _pungButton->setTitleColor(Color3B::BLACK);
    this->addChild(_pungButton);
    _pungButton->setPosition(Vec2(tilesSize.width - 40, tableBottom + 140));
    _pungButton->addClickEventListener(std::bind(&TilePickWidget::pungCallback, this, std::placeholders::_1));

    _meldedKongButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
    _meldedKongButton->setScale9Enabled(true);
    _meldedKongButton->setContentSize(Size(40.0f, 25.0f));
    _meldedKongButton->setTitleText("明杠");
    _meldedKongButton->setTitleColor(Color3B::BLACK);
    this->addChild(_meldedKongButton);
    _meldedKongButton->setPosition(Vec2(tilesSize.width - 40, tableBottom + 100));
    _meldedKongButton->addClickEventListener(std::bind(&TilePickWidget::meldedKongCallback, this, std::placeholders::_1));

    _concealedKongButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
    _concealedKongButton->setScale9Enabled(true);
    _concealedKongButton->setContentSize(Size(40.0f, 25.0f));
    _concealedKongButton->setTitleText("暗杠");
    _concealedKongButton->setTitleColor(Color3B::BLACK);
    this->addChild(_concealedKongButton);
    _concealedKongButton->setPosition(Vec2(tilesSize.width - 40, tableBottom + 60));
    _concealedKongButton->addClickEventListener(std::bind(&TilePickWidget::concealedKongCallback, this, std::placeholders::_1));

    ui::Button *sortButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
    sortButton->setScale9Enabled(true);
    sortButton->setContentSize(Size(40.0f, 25.0f));
    sortButton->setTitleText("排序");
    sortButton->setTitleColor(Color3B::BLACK);
    this->addChild(sortButton);
    sortButton->setPosition(Vec2(tilesSize.width - 90, tableBottom + 20));
    sortButton->addClickEventListener([this](Ref *) { sort(); });

    ui::Button *clearButton = ui::Button::create("source_material/btn_square_normal.png", "source_material/btn_square_selected.png", "source_material/btn_square_disabled.png");
    clearButton->setScale9Enabled(true);
    clearButton->setContentSize(Size(40.0f, 25.0f));
    clearButton->setTitleText("重置");
    clearButton->setTitleColor(Color3B::BLACK);
    this->addChild(clearButton);
    clearButton->setPosition(Vec2(tilesSize.width - 40, tableBottom + 20));
    clearButton->addClickEventListener([this](Ref *) { reset(); });

    _tiles.reserve(13);
    _fixedSets.reserve(4);
    _winTileButton = nullptr;

    reset();

    return true;
}

void TilePickWidget::reset() {
    memset(_totalTilesTable, 0, sizeof(_totalTilesTable));
    memset(_handTilesTable, 0, sizeof(_handTilesTable));
    _winTile = 0;
    _currentIdx = 0;

    for (int i = 0; i < 9; ++i) {
        _characterButtons[i]->setEnabled(true);
        _bambooButtons[i]->setEnabled(true);
        _dotsButtons[i]->setEnabled(true);
    }
    for (int i = 0; i < 7; ++i) {
        _honorButtons[i]->setEnabled(true);
    }

    _tiles.clear();
    _fixedSets.clear();

    while (!_tileButtons.empty()) {
        _tilesWidget->removeChild(_tileButtons.back());
        _tileButtons.pop_back();
    }
    _tilesWidget->removeChild(_winTileButton);
    _winTileButton = nullptr;

    _chowLessButton->setEnabled(false);
    _chowMidButton->setEnabled(false);
    _chowGreatButton->setEnabled(false);
    _pungButton->setEnabled(false);
    _meldedKongButton->setEnabled(false);
    _concealedKongButton->setEnabled(false);
}

void TilePickWidget::sort() {
    std::sort(_tiles.begin(), _tiles.end());
    refreshHandTiles();
}

void TilePickWidget::addOneTile(TILE tile, bool isWinTile) {
    unsigned tilesCnt = _tiles.size();
    ui::Button *button = ui::Button::create(imageName[tile]);
    _tilesWidget->addChild(button);
    button->setTag(tile);

    Vec2 pos;
    pos.y = 19.5f;
    switch (_fixedSets.size()) {
    default: pos.x = 27 * (tilesCnt + 0.5f); break;
    case 1: pos.x = 27 * (tilesCnt + 2); break;
    case 2: pos.x = 27 * (tilesCnt + 3.5f); break;
    case 3: pos.x = 27 * (tilesCnt + 5); break;
    case 4: pos.x = 27 * (tilesCnt + 6.5f); break;
    }

    if (!isWinTile) {
        button->setPosition(pos);
        _tileButtons.push_back(button);
        _tiles.push_back(tile);
        ++_handTilesTable[tile];
    }
    else {
        _winTileButton = button;
        _winTile = tile;
        _winTileButton->setPosition(Vec2(pos.x + 4, pos.y));
    }

    button->addClickEventListener([this, tilesCnt](Ref *) {
        _currentIdx = tilesCnt;
        refreshActionButtons();
    });
    _currentIdx = tilesCnt;
}

void TilePickWidget::tileTableCallback(cocos2d::Ref *sender, TILE tile) {
    unsigned tilesCnt = _tiles.size();
    unsigned cnt = _fixedSets.size() * 3 + tilesCnt;
    if (cnt == 13 && _winTile != 0) {
        return;
    }
    if (++_totalTilesTable[tile] >= 4) {
        SUIT_TYPE suit = tile_suit(tile);
        RANK_TYPE rank = tile_rank(tile);
        switch (suit) {
        case TILE_SUIT_CHARACTERS: _characterButtons[rank - 1]->setEnabled(false); break;
        case TILE_SUIT_BAMBOO: _bambooButtons[rank - 1]->setEnabled(false); break;
        case TILE_SUIT_DOTS: _dotsButtons[rank - 1]->setEnabled(false); break;
        case TILE_SUIT_WINDS: _honorButtons[rank - 1]->setEnabled(false); break;
        case TILE_SUIT_DRAGONS: _honorButtons[rank + 3]->setEnabled(false); break;
        default: break;
        }
    }
    addOneTile(tile, cnt == 13);
    refreshActionButtons();
}

void TilePickWidget::refreshActionButtons() {
    if (_currentIdx >= _tiles.size()) {
        if (_winTileButton != nullptr) {
            _highlightBox->setPosition(_winTileButton->getPosition());
        }

        _chowLessButton->setEnabled(false);
        _chowMidButton->setEnabled(false);
        _chowGreatButton->setEnabled(false);
        _pungButton->setEnabled(false);
        _meldedKongButton->setEnabled(false);
        _concealedKongButton->setEnabled(false);
    }
    else {
        _highlightBox->setPosition(_tileButtons[_currentIdx]->getPosition());

        TILE tile = _tiles[_currentIdx];
        switch (_handTilesTable[tile]) {
        case 4:
            _meldedKongButton->setEnabled(true);
            _concealedKongButton->setEnabled(true);
            _pungButton->setEnabled(true);
            break;
        case 3:
            _meldedKongButton->setEnabled(false);
            _concealedKongButton->setEnabled(false);
            _pungButton->setEnabled(true);
            break;
        default:
            _meldedKongButton->setEnabled(false);
            _concealedKongButton->setEnabled(false);
            _pungButton->setEnabled(false);
        }

        if (!is_honor(tile)) {
            _chowLessButton->setEnabled(_handTilesTable[tile - 2] > 0 && _handTilesTable[tile - 1]);
            _chowMidButton->setEnabled(_handTilesTable[tile - 1] > 0 && _handTilesTable[tile + 1]);
            _chowGreatButton->setEnabled(_handTilesTable[tile + 1] > 0 && _handTilesTable[tile + 2]);
        }
        else {
            _chowLessButton->setEnabled(false);
            _chowMidButton->setEnabled(false);
            _chowGreatButton->setEnabled(false);
        }
    }
}

void TilePickWidget::refreshAfterAction(int meldedIdx) {
    const Size &fixedSize = _fixedWidget->getContentSize();
    Vec2 pos = Vec2(fixedSize.width * 0.25f, fixedSize.height * 0.25f);
    switch (_fixedSets.size()) {
    case 1: pos.y *= 3; break;
    case 2: pos.x *= 3; pos.y *= 3; break;
    case 3: break;
    case 4: pos.x *= 3; break;
    default: return;
    }

    const SET &set = _fixedSets.back();
    switch (set.set_type) {
    case SET_TYPE::CHOW: addFixedChowSet(pos, set.mid_tile, meldedIdx); break;
    case SET_TYPE::PUNG: addFixedPungSet(pos, set.mid_tile, meldedIdx); break;
    case SET_TYPE::KONG:
        if (set.is_melded) {
            addFixedMeldedKongSet(pos, set.mid_tile, meldedIdx);
        }
        else {
            addFixedConcealedKongSet(pos, set.mid_tile);
        }
        break;
    default: return;
    }

    refreshHandTiles();
}

void TilePickWidget::refreshHandTiles() {
    while (!_tileButtons.empty()) {
        _tilesWidget->removeChild(_tileButtons.back());
        _tileButtons.pop_back();
    }

    std::vector<TILE> temp;
    temp.swap(_tiles);
    std::for_each(std::begin(temp), std::end(temp), std::bind(&TilePickWidget::addOneTile, this, std::placeholders::_1, false));
    refreshActionButtons();
}

void TilePickWidget::addFixedChowSet(const cocos2d::Vec2 &center, TILE tile, int meldedIdx) {
    // 一张牌的尺寸：27 * 39
    const char *image[3];
    switch (meldedIdx) {
    default:
        image[0] = imageName[tile - 1];
        image[1] = imageName[tile];
        image[2] = imageName[tile + 1];
        break;
    case 1:
        image[0] = imageName[tile];
        image[1] = imageName[tile - 1];
        image[2] = imageName[tile + 1];
        break;
    case 2:
        image[0] = imageName[tile + 1];
        image[1] = imageName[tile - 1];
        image[2] = imageName[tile];
        break;
    }

    Vec2 pos[3];
    pos[0] = Vec2(center.x - 27, center.y - 6);
    pos[1] = Vec2(center.x + 6, center.y);
    pos[2] = Vec2(center.x + 33, center.y);

    for (int i = 0; i < 3; ++i) {
        Sprite *sprite = Sprite::create(image[i]);
        _fixedWidget->addChild(sprite);
        sprite->setPosition(pos[i]);
        if (i == 0) {
            sprite->setRotation(-90);
        }
    }
}

void TilePickWidget::addFixedPungSet(const cocos2d::Vec2 &center, TILE tile, int meldedIdx) {
    // 一张牌的尺寸：27 * 39
    Vec2 pos[3];
    switch (meldedIdx) {
    default:
        pos[0] = Vec2(center.x - 27, center.y - 6);
        pos[1] = Vec2(center.x + 6, center.y);
        pos[2] = Vec2(center.x + 33, center.y);
        break;
    case 1:
        pos[0] = Vec2(center.x - 33, center.y);
        pos[1] = Vec2(center.x, center.y - 6);
        pos[2] = Vec2(center.x + 33, center.y);
        break;
    case 2:
        pos[0] = Vec2(center.x - 33, center.y);
        pos[1] = Vec2(center.x - 6, center.y);
        pos[2] = Vec2(center.x + 27, center.y - 6);
        break;
    }

    for (int i = 0; i < 3; ++i) {
        Sprite *sprite = Sprite::create(imageName[tile]);
        _fixedWidget->addChild(sprite);
        sprite->setPosition(pos[i]);
        if (i == meldedIdx) {
            sprite->setRotation(-90);
        }
    }
}

void TilePickWidget::addFixedMeldedKongSet(const cocos2d::Vec2 &center, TILE tile, int meldedIdx) {
    // 一张牌的尺寸：27 * 39
    Vec2 pos[4];
    switch (meldedIdx) {
    default:
        pos[0] = Vec2(center.x - 40.5f, center.y - 6);
        pos[1] = Vec2(center.x -  7.5f, center.y);
        pos[2] = Vec2(center.x + 19.5f, center.y);
        pos[3] = Vec2(center.x + 46.5f, center.y);
        break;
    case 1:
        pos[0] = Vec2(center.x - 46.5f, center.y);
        pos[1] = Vec2(center.x - 13.5f, center.y - 6);
        pos[2] = Vec2(center.x + 19.5f, center.y);
        pos[3] = Vec2(center.x + 46.5f, center.y);
        break;
    case 2:
        pos[0] = Vec2(center.x - 46.5f, center.y);
        pos[1] = Vec2(center.x - 19.5f, center.y);
        pos[2] = Vec2(center.x + 13.5f, center.y - 6);
        pos[3] = Vec2(center.x + 46.5f, center.y);
        break;
    case 3:
        pos[0] = Vec2(center.x - 46.5f, center.y);
        pos[1] = Vec2(center.x - 19.5f, center.y);
        pos[2] = Vec2(center.x +  7.5f, center.y);
        pos[3] = Vec2(center.x + 40.5f, center.y - 6);
        break;
    }

    for (int i = 0; i < 4; ++i) {
        Sprite *sprite = Sprite::create(imageName[tile]);
        _fixedWidget->addChild(sprite);
        sprite->setPosition(pos[i]);
        if (i == meldedIdx) {
            sprite->setRotation(-90);
        }
    }
}

void TilePickWidget::addFixedConcealedKongSet(const cocos2d::Vec2 &center, TILE tile) {
    // 一张牌的尺寸：27 * 39
    const char *image[4];
    image[0] = imageName[0x53];
    image[1] = imageName[tile];
    image[2] = imageName[tile];
    image[3] = imageName[0x53];

    Vec2 pos[4];
    pos[0] = Vec2(center.x - 40.5f, center.y);
    pos[1] = Vec2(center.x - 13.5f, center.y);
    pos[2] = Vec2(center.x + 13.5f, center.y);
    pos[3] = Vec2(center.x + 40.5f, center.y);

    for (int i = 0; i < 4; ++i) {
        Sprite *sprite = Sprite::create(image[i]);
        _fixedWidget->addChild(sprite);
        sprite->setPosition(pos[i]);
        if (i == 0 || i == 3) {
            sprite->setColor(Color3B::RED);
        }
    }
}

void TilePickWidget::chowLessCallback(cocos2d::Ref *sender) {
    // XX_
    TILE tile = _tiles[_currentIdx];
    auto it = std::find(std::begin(_tiles), std::end(_tiles), tile - 2);
    _tiles.erase(it);
    it = std::find(std::begin(_tiles), std::end(_tiles), tile - 1);
    _tiles.erase(it);
    it = std::find(std::begin(_tiles), std::end(_tiles), tile);
    _tiles.erase(it);

    --_handTilesTable[tile - 2];
    --_handTilesTable[tile - 1];
    --_handTilesTable[tile];

    SET set;
    set.is_melded = true;
    set.mid_tile = tile - 1;
    set.set_type = SET_TYPE::CHOW;
    _fixedSets.push_back(set);

    refreshAfterAction(2);
}

void TilePickWidget::chowMidCallback(cocos2d::Ref *sender) {
    // X_X
    TILE tile = _tiles[_currentIdx];
    auto it = std::find(std::begin(_tiles), std::end(_tiles), tile - 1);
    _tiles.erase(it);
    it = std::find(std::begin(_tiles), std::end(_tiles), tile);
    _tiles.erase(it);
    it = std::find(std::begin(_tiles), std::end(_tiles), tile + 1);
    _tiles.erase(it);

    --_handTilesTable[tile - 1];
    --_handTilesTable[tile];
    --_handTilesTable[tile + 1];

    SET set;
    set.is_melded = true;
    set.mid_tile = tile;
    set.set_type = SET_TYPE::CHOW;
    _fixedSets.push_back(set);

    refreshAfterAction(1);
}

void TilePickWidget::chowGreatCallback(cocos2d::Ref *sender) {
    // _XX
    TILE tile = _tiles[_currentIdx];
    auto it = std::find(std::begin(_tiles), std::end(_tiles), tile);
    _tiles.erase(it);
    it = std::find(std::begin(_tiles), std::end(_tiles), tile + 1);
    _tiles.erase(it);
    it = std::find(std::begin(_tiles), std::end(_tiles), tile + 2);
    _tiles.erase(it);

    --_handTilesTable[tile];
    --_handTilesTable[tile + 1];
    --_handTilesTable[tile + 2];

    SET set;
    set.is_melded = true;
    set.mid_tile = tile + 1;
    set.set_type = SET_TYPE::CHOW;
    _fixedSets.push_back(set);

    refreshAfterAction(0);
}

void TilePickWidget::pungCallback(cocos2d::Ref *sender) {
    int meldedIdx = 2;
    TILE tile = _tiles[_currentIdx];
    std::vector<TILE>::iterator it[3];
    it[0] = std::find(_tiles.begin(), _tiles.end(), tile);
    it[1] = std::find(it[0] + 1, _tiles.end(), tile);
    it[2] = std::find(it[1] + 1, _tiles.end(), tile);
    for (int i = 3; i-- > 0; ) {
        if (it[i] - _tiles.begin() == _currentIdx) {
            meldedIdx = i;
        }
        _tiles.erase(it[i]);
    }

    _handTilesTable[tile] -= 3;

    SET set;
    set.is_melded = true;
    set.mid_tile = tile;
    set.set_type = SET_TYPE::PUNG;
    _fixedSets.push_back(set);

    refreshAfterAction(meldedIdx);
}

void TilePickWidget::meldedKongCallback(cocos2d::Ref *sender) {
    int meldedIdx = 0;
    TILE tile = _tiles[_currentIdx];
    std::vector<TILE>::iterator it[4];
    it[0] = std::find(_tiles.begin(), _tiles.end(), tile);
    it[1] = std::find(it[0] + 1, _tiles.end(), tile);
    it[2] = std::find(it[1] + 1, _tiles.end(), tile);
    it[3] = std::find(it[2] + 1, _tiles.end(), tile);
    for (int i = 4; i-- > 0; ) {
        if (it[i] - _tiles.begin() == _currentIdx) {
            meldedIdx = i;
        }
        _tiles.erase(it[i]);
    }

    _handTilesTable[tile] -= 4;

    SET set;
    set.is_melded = true;
    set.mid_tile = tile;
    set.set_type = SET_TYPE::KONG;
    _fixedSets.push_back(set);

    refreshAfterAction(meldedIdx);
}

void TilePickWidget::concealedKongCallback(cocos2d::Ref *sender) {
    TILE tile = _tiles[_currentIdx];
    std::vector<TILE>::iterator it[4];
    it[0] = std::find(_tiles.begin(), _tiles.end(), tile);
    it[1] = std::find(it[0] + 1, _tiles.end(), tile);
    it[2] = std::find(it[1] + 1, _tiles.end(), tile);
    it[3] = std::find(it[2] + 1, _tiles.end(), tile);
    for (int i = 4; i-- > 0; ) {
        _tiles.erase(it[i]);
    }

    _handTilesTable[tile] -= 4;

    SET set;
    set.is_melded = false;
    set.mid_tile = tile;
    set.set_type = SET_TYPE::KONG;
    _fixedSets.push_back(set);

    refreshAfterAction(0);
}
