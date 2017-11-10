#include "HandTilesWidget.h"
#include <algorithm>
#include <iterator>
#include "../mahjong-algorithm/fan_calculator.h"
#include "../utils/compiler.h"
#include "../TilesImage.h"

USING_NS_CC;

#define GAP 4

bool HandTilesWidget::init() {
    if (UNLIKELY(!Node::init())) {
        return false;
    }

    this->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    this->setIgnoreAnchorPointForPosition(false);

    // 14张牌宽度：TILE_WIDTH * 14
    Size standingSize = Size(TILE_WIDTH * 14 + GAP, TILE_HEIGHT);
    _standingContainer = Node::create();
    _standingContainer->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    _standingContainer->setIgnoreAnchorPointForPosition(false);
    _standingContainer->setContentSize(standingSize);
    this->addChild(_standingContainer);
    _standingContainer->setPosition(Vec2(standingSize.width * 0.5f, standingSize.height * 0.5f));

    // 高亮方框
    _highlightBox = DrawNode::create();
    _highlightBox->setContentSize(Size(TILE_WIDTH, TILE_HEIGHT));
    _highlightBox->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    _highlightBox->drawRect(Vec2(0.0f, 0.0f), Vec2(TILE_WIDTH, TILE_HEIGHT), Color4F::RED);
    _standingContainer->addChild(_highlightBox, 2);

    const float fixedHeight = TILE_HEIGHT;  // NOTE: 如果将来支持加杠，则应为std::max(TILE_HEIGHT, TILE_WIDTH * 2)
    _fixedContainer = Node::create();
    _fixedContainer->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    _fixedContainer->setIgnoreAnchorPointForPosition(false);
    _fixedContainer->setContentSize(Size(0, fixedHeight));
    this->addChild(_fixedContainer);
    _fixedContainer->setPosition(Vec2(standingSize.width * 0.5f, standingSize.height + fixedHeight * 0.5f + GAP));

    this->setContentSize(Size(standingSize.width, standingSize.height + fixedHeight + GAP));

    _standingTiles.reserve(14);
    _fixedPacks.reserve(4);

    reset();

    return true;
}

void HandTilesWidget::reset() {
    // 清空数据
    memset(_usedTilesTable, 0, sizeof(_usedTilesTable));
    memset(_standingTilesTable, 0, sizeof(_standingTilesTable));
    _currentIdx = 0;
    _standingTiles.clear();
    _fixedPacks.clear();

    // 清除之前的残留的立牌
    while (!_standingTileButtons.empty()) {
        _standingContainer->removeChild(_standingTileButtons.back());
        _standingTileButtons.pop_back();
    }
    _standingContainer->setContentSize(Size(TILE_WIDTH * 14 + GAP, TILE_HEIGHT));
    _highlightBox->setPosition(Vec2(TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f));

    // 清除之前残留的副露
    _fixedContainer->removeAllChildren();
    _fixedContainer->setContentSize(Size(0, _fixedContainer->getContentSize().height));
    _fixedContainer->setScale(1);
}

void HandTilesWidget::setData(const mahjong::hand_tiles_t &handTiles, mahjong::tile_t servingTile) {
    reset();

    // 添加副露
    for (long i = 0; i < handTiles.pack_count; ++i) {
        mahjong::pack_t pack = handTiles.fixed_packs[i];
        _fixedPacks.push_back(pack);
        mahjong::tile_t tile = mahjong::pack_get_tile(pack);
        switch (mahjong::pack_get_type(pack)) {
        case PACK_TYPE_CHOW:
            switch (mahjong::pack_get_offer(pack)) {
            default: addFixedChowPack(tile - 1, 0); break;
            case 2: addFixedChowPack(tile, 1); break;
            case 3: addFixedChowPack(tile + 1, 2); break;
            }
            ++_usedTilesTable[tile - 1];
            ++_usedTilesTable[tile];
            ++_usedTilesTable[tile + 1];
            break;
        case PACK_TYPE_PUNG:
            switch (mahjong::pack_get_offer(pack)) {
            default: addFixedPungPack(tile, 0); break;
            case 2: addFixedPungPack(tile, 1); break;
            case 3: addFixedPungPack(tile, 2); break;
            }
            _usedTilesTable[tile] += 3;
            break;
        case PACK_TYPE_KONG:
            switch (mahjong::pack_get_offer(pack)) {
            case 0: addFixedConcealedKongPack(tile); break;
            default: addFixedMeldedKongPack(tile, 0); break;
            case 2: addFixedMeldedKongPack(tile, 1); break;
            case 3: addFixedMeldedKongPack(tile, 3); break;
            }
            _usedTilesTable[tile] += 4;
            break;
        default:
            break;
        }
    }

    refreshStandingTilesPos();

    // 添加立牌
    for (long i = 0; i < handTiles.tile_count; ++i) {
        mahjong::tile_t tile = handTiles.standing_tiles[i];
        addTile(tile);
        ++_usedTilesTable[tile];
    }

    if (servingTile != 0) {
        addTile(servingTile);
        ++_usedTilesTable[servingTile];
    }

    refreshHighlightPos();
}

void HandTilesWidget::getData(mahjong::hand_tiles_t *handTiles, mahjong::tile_t *servingTile) const {
    // 获取上的牌
    mahjong::tile_t st = getServingTile();
    *servingTile = st;

    // 获取副露
    handTiles->pack_count = std::copy(_fixedPacks.begin(), _fixedPacks.end(), std::begin(handTiles->fixed_packs))
        - std::begin(handTiles->fixed_packs);

    size_t maxCnt = 13 - _fixedPacks.size() * 3;  // 立牌数最大值（不包括和牌）
    // 获取立牌
    size_t cnt = std::copy(_standingTiles.begin(), _standingTiles.end() - (st == 0 ? 0 : 1), std::begin(handTiles->standing_tiles))
            - std::begin(handTiles->standing_tiles);
    handTiles->tile_count = std::min(maxCnt, cnt);
}

mahjong::tile_t HandTilesWidget::getServingTile() const {
    size_t maxCnt = 13 - _fixedPacks.size() * 3;  // 立牌数最大值（不包括和牌）
    if (_standingTiles.size() < maxCnt + 1) {
        return 0;
    }
    return _standingTiles.back();
}

bool HandTilesWidget::isFixedPacksContainsKong() const {
    return mahjong::is_fixed_packs_contains_kong(_fixedPacks.data(), _fixedPacks.size());
}

bool HandTilesWidget::isStandingTilesContainsServingTile() const {
    mahjong::tile_t servingTile = getServingTile();
    if (servingTile == 0) {
        return false;
    }
    return mahjong::is_standing_tiles_contains_win_tile(
        _standingTiles.data(), _standingTiles.size() - 1, servingTile);
}

size_t HandTilesWidget::countServingTileInFixedPacks() const {
    mahjong::tile_t servingTile = getServingTile();
    if (servingTile == 0 || _fixedPacks.empty()) {
        return 0;
    }

    return mahjong::count_win_tile_in_fixed_packs(
        _fixedPacks.data(), _fixedPacks.size(), servingTile);
}

void HandTilesWidget::onTileButton(cocos2d::Ref *sender) {
    ui::Button *button = (ui::Button *)sender;
    _currentIdx = reinterpret_cast<size_t>(button->getUserData());
    refreshHighlightPos();
    if (_tileClickCallback) {
        _tileClickCallback();
    }
}

// 添加一张牌
void HandTilesWidget::addTile(mahjong::tile_t tile) {
    ui::Button *button = ui::Button::create(tilesImageName[tile]);
    button->setScale(CC_CONTENT_SCALE_FACTOR());
    _standingContainer->addChild(button);

    size_t tilesCnt = _standingTiles.size();
    button->setUserData(reinterpret_cast<void *>(tilesCnt));
    button->setTag(tile);
    button->addClickEventListener(std::bind(&HandTilesWidget::onTileButton, this, std::placeholders::_1));

    Vec2 pos = Vec2(TILE_WIDTH * (tilesCnt + 0.5f), TILE_HEIGHT * 0.5f);
    size_t maxCnt = 13 - _fixedPacks.size() * 3;  // 立牌数最大值（不包括和牌）
    if (LIKELY(tilesCnt < maxCnt)) {
        button->setPosition(pos);
        _currentIdx = tilesCnt + 1;
    }
    else {
        button->setPosition(Vec2(pos.x + GAP, pos.y));  // 和牌张与立牌间隔4像素
    }
    ++_standingTilesTable[tile];

    _standingTileButtons.push_back(button);
    _standingTiles.push_back(tile);
}

// 替换一张牌
void HandTilesWidget::replaceTile(mahjong::tile_t tile) {
    // 直接换button的图
    ui::Button *button = _standingTileButtons[_currentIdx];
    button->loadTextureNormal(tilesImageName[tile]);
    button->setTag(tile);

    mahjong::tile_t prevTile = _standingTiles[_currentIdx];
    --_standingTilesTable[prevTile];

    _standingTiles[_currentIdx] = tile;
    ++_standingTilesTable[tile];

    size_t maxCnt = 13 - _fixedPacks.size() * 3;  // 立牌数最大值（不包括和牌）
    if (_currentIdx < maxCnt) {
        ++_currentIdx;
    }
}

mahjong::tile_t HandTilesWidget::putTile(mahjong::tile_t tile) {
    mahjong::tile_t prevTile = 0;
    if (_currentIdx >= _standingTiles.size()) {  // 新增牌
        addTile(tile);
        ++_usedTilesTable[tile];
    }
    else {  // 修改牌
        prevTile = _standingTiles[_currentIdx];  // 此位置之前的牌
        if (prevTile != tile) {  // 新选的牌与之前的牌不同，更新相关信息
            replaceTile(tile);
            --_usedTilesTable[prevTile];
            ++_usedTilesTable[tile];
        }
        else {
            size_t maxCnt = 13 - _fixedPacks.size() * 3;  // 立牌数最大值（不包括和牌）
            if (_currentIdx < maxCnt) {
                ++_currentIdx;
            }
        }
    }

    refreshHighlightPos();
    return prevTile;
}

// 刷新高亮位置
void HandTilesWidget::refreshHighlightPos() {
    size_t maxCnt = 13 - _fixedPacks.size() * 3;  // 立牌数最大值（不包括和牌）
    Vec2 pos = Vec2(TILE_WIDTH * (_currentIdx + 0.5f), TILE_HEIGHT * 0.5f);
    if (LIKELY(_currentIdx < maxCnt)) {
        _highlightBox->setPosition(pos);
    }
    else {
        _highlightBox->setPosition(Vec2(pos.x + GAP, pos.y));
    }

    if (_currentIdxChangedCallback) {
        _currentIdxChangedCallback();
    }
}

// 刷新立牌
void HandTilesWidget::refreshStandingTiles() {
    // 遍历立牌
    for (size_t i = 0, cnt = _standingTileButtons.size(); i < cnt; ) {
        ui::Button *button = _standingTileButtons[i];
        mahjong::tile_t tile = static_cast<mahjong::tile_t>(button->getTag());
        if (i >= _standingTiles.size() || tile != _standingTiles[i]) {  // 已经被删除的
            _standingContainer->removeChild(button);
            _standingTileButtons.erase(_standingTileButtons.begin() + i);
            --cnt;
        }
        else {
            ++i;
        }
    }

    //assert(_standingTiles.size() == _standingTileButtons.size());

    refreshStandingTilesPos();

    // 更新currentIdx
    size_t prevIndex = _currentIdx;
    _currentIdx = _standingTiles.size();
    if (prevIndex < _currentIdx) {
        _currentIdx = prevIndex;
    }

    size_t maxCnt = 13 - _fixedPacks.size() * 3;  // 立牌数最大值（不包括和牌）
    if (_currentIdx > maxCnt) {
        _currentIdx = maxCnt;
    }
    refreshHighlightPos();
}

// 刷新立牌位置
void HandTilesWidget::refreshStandingTilesPos() {
    size_t maxCnt = 13 - _fixedPacks.size() * 3;  // 立牌数最大值（不包括和牌）
    _standingContainer->setContentSize(Size(static_cast<float>(TILE_WIDTH * (maxCnt + 1) + GAP), TILE_HEIGHT));

    // 重新设置UserData及位置
    for (size_t i = 0, cnt = _standingTileButtons.size(); i < cnt; ++i) {
        ui::Button *button = _standingTileButtons[i];
        button->setUserData(reinterpret_cast<void *>(i));

        Vec2 pos = Vec2(TILE_WIDTH * (i + 0.5f), TILE_HEIGHT * 0.5f);
        if (LIKELY(i < maxCnt)) {
            button->setPosition(pos);
        }
        else {
            button->setPosition(Vec2(pos.x + GAP, pos.y));
        }
    }
}

// 排序立牌
void HandTilesWidget::sortStandingTiles() {
    if (UNLIKELY(_standingTiles.empty())) {
        return;
    }

    const size_t offset = getServingTile() == 0 ? 0 : 1;

    std::sort(_standingTiles.begin(), _standingTiles.end() - offset);
    std::sort(_standingTileButtons.begin(), _standingTileButtons.end() - offset, [](ui::Button *btn1, ui::Button *btn2) {
        return btn1->getTag() < btn2->getTag();
    });

    refreshStandingTilesPos();
}

// 添加一组吃
void HandTilesWidget::addFixedChowPack(mahjong::tile_t tile, int meldedIdx) {
    const Size fixedSize = _fixedContainer->getContentSize();
    const float offsetX = _fixedPacks.size() > 1 ? GAP : 0.0f;
    const float startX = fixedSize.width + offsetX;
    const float totalWidth = fixedSize.width + offsetX + TILE_HEIGHT + TILE_WIDTH * 2;
    _fixedContainer->setContentSize(Size(totalWidth, fixedSize.height));
    const float maxWidth = TILE_WIDTH * 14 + GAP;
    if (totalWidth > maxWidth) {
        _fixedContainer->setScale(maxWidth / totalWidth);
    }

    const char *image[3];
    switch (meldedIdx) {
    default:
        image[0] = tilesImageName[tile];
        image[1] = tilesImageName[tile + 1];
        image[2] = tilesImageName[tile + 2];
        break;
    case 1:
        image[0] = tilesImageName[tile];
        image[1] = tilesImageName[tile - 1];
        image[2] = tilesImageName[tile + 1];
        break;
    case 2:
        image[0] = tilesImageName[tile];
        image[1] = tilesImageName[tile - 2];
        image[2] = tilesImageName[tile - 1];
        break;
    }

    Vec2 pos[3];
    pos[0] = Vec2(startX + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
    pos[1] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
    pos[2] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);

    const float contentScaleFactor = CC_CONTENT_SCALE_FACTOR();
    for (int i = 0; i < 3; ++i) {
        Sprite *sprite = Sprite::create(image[i]);
        sprite->setScale(contentScaleFactor);
        _fixedContainer->addChild(sprite);
        sprite->setPosition(pos[i]);
        if (i == 0) {
            sprite->setRotation(-90);
        }
    }
}

// 添加一组碰
void HandTilesWidget::addFixedPungPack(mahjong::tile_t tile, int meldedIdx) {
    const Size fixedSize = _fixedContainer->getContentSize();
    const float offsetX = _fixedPacks.size() > 1 ? GAP : 0.0f;
    const float startX = fixedSize.width + offsetX;
    const float totalWidth = fixedSize.width + offsetX + TILE_HEIGHT + TILE_WIDTH * 2;
    _fixedContainer->setContentSize(Size(totalWidth, fixedSize.height));
    const float maxWidth = TILE_WIDTH * 14 + GAP;
    if (totalWidth > maxWidth) {
        _fixedContainer->setScale(maxWidth / totalWidth);
    }

    Vec2 pos[3];
    switch (meldedIdx) {
    default:
        pos[0] = Vec2(startX + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
        pos[1] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        pos[2] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
        break;
    case 1:
        pos[0] = Vec2(startX + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        pos[1] = Vec2(startX + TILE_WIDTH + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
        pos[2] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
        break;
    case 2:
        pos[0] = Vec2(startX + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        pos[1] = Vec2(startX + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
        pos[2] = Vec2(startX + TILE_WIDTH * 2 + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
        break;
    }

    const float contentScaleFactor = CC_CONTENT_SCALE_FACTOR();
    for (int i = 0; i < 3; ++i) {
        Sprite *sprite = Sprite::create(tilesImageName[tile]);
        sprite->setScale(contentScaleFactor);
        _fixedContainer->addChild(sprite);
        sprite->setPosition(pos[i]);
        if (i == meldedIdx) {
            sprite->setRotation(-90);
        }
    }
}

// 添加一组明杠
void HandTilesWidget::addFixedMeldedKongPack(mahjong::tile_t tile, int meldedIdx) {
    const Size fixedSize = _fixedContainer->getContentSize();
    const float offsetX = _fixedPacks.size() > 1 ? GAP : 0.0f;
    const float startX = fixedSize.width + offsetX;
    const float totalWidth = fixedSize.width + offsetX + TILE_HEIGHT + TILE_WIDTH * 3;
    _fixedContainer->setContentSize(Size(totalWidth, fixedSize.height));
    const float maxWidth = TILE_WIDTH * 14 + GAP;
    if (totalWidth > maxWidth) {
        _fixedContainer->setScale(maxWidth / totalWidth);
    }

    Vec2 pos[4];
    switch (meldedIdx) {
    default:
        pos[0] = Vec2(startX + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
        pos[1] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        pos[2] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
        pos[3] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);
        break;
    case 1:
    case 2:
        pos[0] = Vec2(startX + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        pos[1] = Vec2(startX + TILE_WIDTH + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
        pos[2] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
        pos[3] = Vec2(startX + TILE_HEIGHT + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);
        break;
    case 3:
        pos[0] = Vec2(startX + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        pos[1] = Vec2(startX + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
        pos[2] = Vec2(startX + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);
        pos[3] = Vec2(startX + TILE_WIDTH * 3 + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
        break;
    }

    const float contentScaleFactor = CC_CONTENT_SCALE_FACTOR();
    for (int i = 0; i < 4; ++i) {
        Sprite *sprite = Sprite::create(tilesImageName[tile]);
        sprite->setScale(contentScaleFactor);
        _fixedContainer->addChild(sprite);
        sprite->setPosition(pos[i]);
        if (i == meldedIdx) {
            sprite->setRotation(-90);
        }
    }
}

// 添加一组暗杠
void HandTilesWidget::addFixedConcealedKongPack(mahjong::tile_t tile) {
    const Size fixedSize = _fixedContainer->getContentSize();
    const float offsetX = _fixedPacks.size() > 1 ? GAP : 0.0f;
    const float startX = fixedSize.width + offsetX;
    const float totalWidth = fixedSize.width + offsetX + TILE_WIDTH * 4;
    _fixedContainer->setContentSize(Size(totalWidth, fixedSize.height));
    const float maxWidth = TILE_WIDTH * 14 + GAP;
    if (totalWidth > maxWidth) {
        _fixedContainer->setScale(maxWidth / totalWidth);
    }

    const char *image[4];
    image[0] = "tiles/bg.png";
    image[1] = tilesImageName[tile];
    image[2] = tilesImageName[tile];
    image[3] = "tiles/bg.png";

    Vec2 pos[4];
    pos[0] = Vec2(startX + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
    pos[1] = Vec2(startX + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
    pos[2] = Vec2(startX + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);
    pos[3] = Vec2(startX + TILE_WIDTH * 3.5f, TILE_HEIGHT * 0.5f);

    const float contentScaleFactor = CC_CONTENT_SCALE_FACTOR();
    for (int i = 0; i < 4; ++i) {
        Sprite *sprite = Sprite::create(image[i]);
        sprite->setScale(contentScaleFactor);
        _fixedContainer->addChild(sprite);
        sprite->setPosition(pos[i]);
    }
}

bool HandTilesWidget::canChow1() const {
    if (_currentIdx >= _standingTiles.size()) {  // 当前位置没有牌
        return false;
    }

    size_t maxCnt = 13 - _fixedPacks.size() * 3;  // 立牌数最大值（不包括和牌）
    if (_currentIdx == maxCnt) {  // 不允许对和牌张进行副露
        return false;
    }

    // _XX 23吃1
    mahjong::tile_t tile = _standingTiles[_currentIdx];
    return (!mahjong::is_honor(tile)
        && _standingTilesTable[tile] > 0
        && _standingTilesTable[tile + 1] > 0
        && _standingTilesTable[tile + 2] > 0);
}

bool HandTilesWidget::canChow2() const {
    if (_currentIdx >= _standingTiles.size()) {  // 当前位置没有牌
        return false;
    }

    size_t maxCnt = 13 - _fixedPacks.size() * 3;  // 立牌数最大值（不包括和牌）
    if (_currentIdx == maxCnt) {  // 不允许对和牌张进行副露
        return false;
    }

    // X_X 13吃2
    mahjong::tile_t tile = _standingTiles[_currentIdx];
    return (!mahjong::is_honor(tile)
        && _standingTilesTable[tile - 1] > 0
        && _standingTilesTable[tile] > 0
        && _standingTilesTable[tile + 1] > 0);
}

bool HandTilesWidget::canChow3() const {
    if (_currentIdx >= _standingTiles.size()) {  // 当前位置没有牌
        return false;
    }

    size_t maxCnt = 13 - _fixedPacks.size() * 3;  // 立牌数最大值（不包括和牌）
    if (_currentIdx == maxCnt) {  // 不允许对和牌张进行副露
        return false;
    }

    // XX_ 12吃3
    mahjong::tile_t tile = _standingTiles[_currentIdx];
    return (!mahjong::is_honor(tile)
        && _standingTilesTable[tile - 2] > 0
        && _standingTilesTable[tile - 1] > 0
        && _standingTilesTable[tile] > 0);
}

bool HandTilesWidget::canPung() const {
    if (_currentIdx >= _standingTiles.size()) {  // 当前位置没有牌
        return false;
    }

    size_t maxCnt = 13 - _fixedPacks.size() * 3;  // 立牌数最大值（不包括和牌）
    if (_currentIdx == maxCnt) {  // 不允许对和牌张进行副露
        return false;
    }

    mahjong::tile_t tile = _standingTiles[_currentIdx];
    return (_standingTilesTable[tile] >= 3);
}

bool HandTilesWidget::canKong() const {
    if (_currentIdx >= _standingTiles.size()) {  // 当前位置没有牌
        return false;
    }

    size_t maxCnt = 13 - _fixedPacks.size() * 3;  // 立牌数最大值（不包括和牌）
    if (_currentIdx == maxCnt) {  // 不允许对和牌张进行副露
        return false;
    }

    mahjong::tile_t tile = _standingTiles[_currentIdx];
    return (_standingTilesTable[tile] >= 4);
}

bool HandTilesWidget::makeFixedChow1Pack() {
    if (UNLIKELY(!canChow1())) {
        return false;
    }

    // _XX 23吃1
    mahjong::tile_t tile = _standingTiles[_currentIdx];
    mahjong::pack_t pack = mahjong::make_pack(1, PACK_TYPE_CHOW, tile + 1);
    _fixedPacks.push_back(pack);

    // 这里迭代器不能连续使用，因为立牌不一定有序
    std::vector<mahjong::tile_t>::iterator it;
    it = std::find(_standingTiles.begin(), _standingTiles.end(), tile);
    _standingTiles.erase(it);
    it = std::find(_standingTiles.begin(), _standingTiles.end(), tile + 1);
    _standingTiles.erase(it);
    it = std::find(_standingTiles.begin(), _standingTiles.end(), tile + 2);
    _standingTiles.erase(it);
    --_standingTilesTable[tile];
    --_standingTilesTable[tile + 1];
    --_standingTilesTable[tile + 2];

    addFixedChowPack(tile, 0);
    refreshStandingTiles();
    return true;
}

bool HandTilesWidget::makeFixedChow2Pack() {
    if (UNLIKELY(!canChow2())) {
        return false;
    }

    // X_X 13吃2
    mahjong::tile_t tile = _standingTiles[_currentIdx];
    mahjong::pack_t pack = mahjong::make_pack(2, PACK_TYPE_CHOW, tile);
    _fixedPacks.push_back(pack);

    // 这里迭代器不能连续使用，因为立牌不一定有序
    std::vector<mahjong::tile_t>::iterator it;
    it = std::find(_standingTiles.begin(), _standingTiles.end(), tile - 1);
    _standingTiles.erase(it);
    it = std::find(_standingTiles.begin(), _standingTiles.end(), tile);
    _standingTiles.erase(it);
    it = std::find(_standingTiles.begin(), _standingTiles.end(), tile + 1);
    _standingTiles.erase(it);
    --_standingTilesTable[tile - 1];
    --_standingTilesTable[tile];
    --_standingTilesTable[tile + 1];

    addFixedChowPack(tile, 1);
    refreshStandingTiles();
    return true;
}

bool HandTilesWidget::makeFixedChow3Pack() {
    if (UNLIKELY(!canChow3())) {
        return false;
    }

    // XX_ 12吃3
    mahjong::tile_t tile = _standingTiles[_currentIdx];
    mahjong::pack_t pack = mahjong::make_pack(3, PACK_TYPE_CHOW, tile - 1);
    _fixedPacks.push_back(pack);

    // 这里迭代器不能连续使用，因为立牌不一定有序
    std::vector<mahjong::tile_t>::iterator it;
    it = std::find(_standingTiles.begin(), _standingTiles.end(), tile - 2);
    _standingTiles.erase(it);
    it = std::find(_standingTiles.begin(), _standingTiles.end(), tile - 1);
    _standingTiles.erase(it);
    it = std::find(_standingTiles.begin(), _standingTiles.end(), tile);
    _standingTiles.erase(it);
    --_standingTilesTable[tile - 2];
    --_standingTilesTable[tile - 1];
    --_standingTilesTable[tile];

    addFixedChowPack(tile, 2);
    refreshStandingTiles();
    return true;
}

int HandTilesWidget::calcMeldedIdx(int maxIdx) const {
    mahjong::tile_t tile = _standingTiles[_currentIdx];

    size_t offset = 0;
    size_t maxCnt = 13 - _fixedPacks.size() * 3;  // 立牌数最大值（不包括和牌）
    if (UNLIKELY(_currentIdx == maxCnt)) {  // 不允许对和牌张进行副露
        offset = 1;
    }

    bool leftExsits = std::any_of(_standingTiles.begin(), _standingTiles.begin() + _currentIdx,
        [tile](mahjong::tile_t t) { return tile == t; });
    bool rightExsits = std::any_of(_standingTiles.begin() + _currentIdx + 1, _standingTiles.end() - offset,
        [tile](mahjong::tile_t t) { return tile == t; });
    return leftExsits ? (rightExsits ? 1 : maxIdx) : 0;
}

bool HandTilesWidget::makeFixedPungPack() {
    if (UNLIKELY(!canPung())) {  // 当前位置没有牌
        return false;
    }

    mahjong::tile_t tile = _standingTiles[_currentIdx];
    int meldedIdx = calcMeldedIdx(2);
    mahjong::pack_t pack = mahjong::make_pack(meldedIdx + 1, PACK_TYPE_PUNG, tile);
    _fixedPacks.push_back(pack);

    // 这里迭代器可以连续使用，因为移除的是同一种牌
    std::vector<mahjong::tile_t>::iterator it = _standingTiles.begin();
    for (int i = 0; i < 3; ++i) {
        it = std::find(it, _standingTiles.end(), tile);
        it = _standingTiles.erase(it);
    }
    _standingTilesTable[tile] -= 3;

    addFixedPungPack(tile, meldedIdx);
    refreshStandingTiles();
    return true;
}

bool HandTilesWidget::makeFixedMeldedKongPack() {
    if (UNLIKELY(!canKong())) {
        return false;
    }

    mahjong::tile_t tile = _standingTiles[_currentIdx];
    int meldedIdx = calcMeldedIdx(3);
    mahjong::pack_t pack = mahjong::make_pack(std::min(meldedIdx + 1, 3), PACK_TYPE_KONG, tile);
    _fixedPacks.push_back(pack);

    // 这里迭代器可以连续使用，因为移除的是同一种牌
    std::vector<mahjong::tile_t>::iterator it = _standingTiles.begin();
    for (int i = 0; i < 4; ++i) {
        it = std::find(it, _standingTiles.end(), tile);
        it = _standingTiles.erase(it);
    }
    _standingTilesTable[tile] -= 4;

    addFixedMeldedKongPack(tile, meldedIdx);
    refreshStandingTiles();
    return true;
}

bool HandTilesWidget::makeFixedConcealedKongPack() {
    if (UNLIKELY(!canKong())) {
        return false;
    }

    mahjong::tile_t tile = _standingTiles[_currentIdx];
    mahjong::pack_t pack = mahjong::make_pack(0, PACK_TYPE_KONG, tile);
    _fixedPacks.push_back(pack);

    // 这里迭代器可以连续使用，因为移除的是同一种牌
    std::vector<mahjong::tile_t>::iterator it = _standingTiles.begin();
    for (int i = 0; i < 4; ++i) {
        it = std::find(it, _standingTiles.end(), tile);
        it = _standingTiles.erase(it);
    }
    _standingTilesTable[tile] -= 4;

    addFixedConcealedKongPack(tile);
    refreshStandingTiles();
    return true;
}

cocos2d::Node *HandTilesWidget::createStaticNode(const mahjong::hand_tiles_t &handTiles, mahjong::tile_t servingTile) {
    const char *image[18];
    Vec2 pos[18];
    bool rotated[18] = { false };
    long tile_cnt = 0;
    int totalWidth = 0;

    // 副露
    for (long i = 0; i < handTiles.pack_count; ++i) {
        mahjong::pack_t pack = handTiles.fixed_packs[i];
        mahjong::tile_t tile = mahjong::pack_get_tile(pack);
        switch (mahjong::pack_get_type(pack)) {
        case PACK_TYPE_CHOW:
            rotated[tile_cnt + 0] = true;

            pos[tile_cnt + 0] = Vec2(totalWidth + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
            pos[tile_cnt + 1] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
            pos[tile_cnt + 2] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);

            switch (mahjong::pack_get_offer(pack)) {
            default:
                image[tile_cnt + 0] = tilesImageName[tile - 1];
                image[tile_cnt + 1] = tilesImageName[tile];
                image[tile_cnt + 2] = tilesImageName[tile + 1];
                break;
            case 2:
                image[tile_cnt + 0] = tilesImageName[tile];
                image[tile_cnt + 1] = tilesImageName[tile - 1];
                image[tile_cnt + 2] = tilesImageName[tile + 1];
                break;
            case 3:
                image[tile_cnt + 0] = tilesImageName[tile + 1];
                image[tile_cnt + 1] = tilesImageName[tile - 1];
                image[tile_cnt + 2] = tilesImageName[tile];
                break;
            }
            totalWidth += (TILE_WIDTH * 2 + TILE_HEIGHT + GAP);
            tile_cnt += 3;
            break;

        case PACK_TYPE_PUNG:
            image[tile_cnt + 0] = tilesImageName[tile];
            image[tile_cnt + 1] = tilesImageName[tile];
            image[tile_cnt + 2] = tilesImageName[tile];

            switch (mahjong::pack_get_offer(pack)) {
            default:
                rotated[tile_cnt + 0] = true;

                pos[tile_cnt + 0] = Vec2(totalWidth + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
                pos[tile_cnt + 1] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 2] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
                break;
            case 2:
                rotated[tile_cnt + 1] = true;

                pos[tile_cnt + 0] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 1] = Vec2(totalWidth + TILE_WIDTH + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
                pos[tile_cnt + 2] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
                break;
            case 3:
                rotated[tile_cnt + 2] = true;

                pos[tile_cnt + 0] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 1] = Vec2(totalWidth + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 2] = Vec2(totalWidth + TILE_WIDTH * 2 + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
                break;
            }
            totalWidth += (TILE_WIDTH * 2 + TILE_HEIGHT + GAP);
            tile_cnt += 3;
            break;

        case PACK_TYPE_KONG:
            image[tile_cnt + 0] = tilesImageName[tile];
            image[tile_cnt + 1] = tilesImageName[tile];
            image[tile_cnt + 2] = tilesImageName[tile];
            image[tile_cnt + 3] = tilesImageName[tile];

            switch (mahjong::pack_get_offer(pack)) {
            case 0:
                image[tile_cnt + 0] = "tiles/bg.png";
                image[tile_cnt + 3] = "tiles/bg.png";

                pos[tile_cnt + 0] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 1] = Vec2(totalWidth + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 2] = Vec2(totalWidth + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 3] = Vec2(totalWidth + TILE_WIDTH * 3.5f, TILE_HEIGHT * 0.5f);

                totalWidth += (TILE_WIDTH * 4 + GAP);
                break;
            default:
                rotated[tile_cnt + 0] = true;

                pos[tile_cnt + 0] = Vec2(totalWidth + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
                pos[tile_cnt + 1] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 2] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 3] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);

                totalWidth += (TILE_WIDTH * 3 + TILE_HEIGHT + GAP);
                break;
            case 2:
                rotated[tile_cnt + 1] = true;

                pos[tile_cnt + 0] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 1] = Vec2(totalWidth + TILE_WIDTH + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);
                pos[tile_cnt + 2] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 3] = Vec2(totalWidth + TILE_HEIGHT + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);

                totalWidth += (TILE_WIDTH * 3 + TILE_HEIGHT + GAP);
                break;
            case 3:
                rotated[tile_cnt + 3] = true;
                pos[tile_cnt + 0] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 1] = Vec2(totalWidth + TILE_WIDTH * 1.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 2] = Vec2(totalWidth + TILE_WIDTH * 2.5f, TILE_HEIGHT * 0.5f);
                pos[tile_cnt + 3] = Vec2(totalWidth + TILE_WIDTH * 3 + TILE_HEIGHT * 0.5f, TILE_WIDTH * 0.5f);

                totalWidth += (TILE_WIDTH * 3 + TILE_HEIGHT + GAP);
                break;
            }
            tile_cnt += 4;
            break;

        default:
            break;
        }
    }

    // 立牌
    for (long i = 0; i < handTiles.tile_count; ++i) {
        mahjong::tile_t tile = handTiles.standing_tiles[i];
        image[tile_cnt] = tilesImageName[tile];
        pos[tile_cnt] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        totalWidth += TILE_WIDTH;
        ++tile_cnt;
    }

    // 上牌
    if (servingTile != 0 && totalWidth > 0) {
        totalWidth += GAP;

        image[tile_cnt] = tilesImageName[servingTile];
        pos[tile_cnt] = Vec2(totalWidth + TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f);
        totalWidth += TILE_WIDTH;
        ++tile_cnt;
    }

    Node *node = Node::create();
    node->setContentSize(Size(static_cast<float>(totalWidth), TILE_HEIGHT));
    node->setIgnoreAnchorPointForPosition(false);
    node->setAnchorPoint(Vec2::ANCHOR_MIDDLE);

    const float contentScaleFactor = CC_CONTENT_SCALE_FACTOR();
    for (long i = 0; i < tile_cnt; ++i) {
        Sprite *sprite = Sprite::create(image[i]);
        sprite->setScale(contentScaleFactor);
        node->addChild(sprite);
        sprite->setPosition(pos[i]);
        if (rotated[i]) {
            sprite->setRotation(-90);
        }
    }

    return node;
}
