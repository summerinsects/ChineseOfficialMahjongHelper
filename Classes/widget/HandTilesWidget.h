#ifndef __HAND_TILES_WIDGETS_H__
#define __HAND_TILES_WIDGETS_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "../mahjong-algorithm/tile.h"

class HandTilesWidget : public cocos2d::ui::Widget {
public:
    CREATE_FUNC(HandTilesWidget);

    const std::vector<mahjong::tile_t> &getStandingTiles() const { return _standingTiles; }
    const std::vector<mahjong::pack_t> &getFixedPacks() const { return _fixedPacks; }

    void setCurrentIdxChangedCallback(const std::function<void ()> &callback) { _currentIdxChangedCallback = callback; }

    size_t getCurrentIdx() const { return _currentIdx; }
    int getUsedTileCount(mahjong::tile_t tile) const { return _usedTilesTable[tile]; }
    int getStandingTileCount(mahjong::tile_t tile) const { return _standingTilesTable[tile]; }

    void setData(const mahjong::hand_tiles_t &handTiles, mahjong::tile_t drawnTile);
    bool getData(mahjong::hand_tiles_t *handTiles, mahjong::tile_t *drawnTile) const;

    mahjong::tile_t getDrawnTile() const;
    bool isFixedPacksContainsKong() const;
    bool isStandingTilesContainsWinTile() const;
    size_t countWinTileInFixedPacks() const;

CC_CONSTRUCTOR_ACCESS:
    virtual bool init() override;

public:
    void reset();
    void sortStandingTiles();
    mahjong::tile_t putTile(mahjong::tile_t tile);

    bool canChow_XX();
    bool canChowX_X();
    bool canChowXX_();
    bool canPung();
    bool canKong();

    bool makeFixedChow_XXPack();
    bool makeFixedChowX_XPack();
    bool makeFixedChowXX_Pack();
    bool makeFixedPungPack();
    bool makeFixedMeldedKongPack();
    bool makeFixedConcealedKongPack();

private:
    cocos2d::ui::Widget *_fixedWidget;
    cocos2d::ui::Widget *_standingWidget;
    std::vector<cocos2d::ui::Button *> _standingTileButtons;
    cocos2d::DrawNode *_highlightBox;

    int _usedTilesTable[mahjong::TILE_TABLE_COUNT];
    int _standingTilesTable[mahjong::TILE_TABLE_COUNT];
    std::vector<mahjong::tile_t> _standingTiles;
    std::vector<mahjong::pack_t> _fixedPacks;
    size_t _currentIdx;

    std::function<void ()> _currentIdxChangedCallback;

private:
    cocos2d::Vec2 calcStandingTilePos(size_t idx) const;
    cocos2d::Vec2 calcFixedPackPos(size_t idx) const;

    void addTile(mahjong::tile_t tile);
    void replaceTile(mahjong::tile_t tile);
    void refreshHighlightPos();
    void refreshStandingTiles();
    void refreshStandingTilesPos();

    void onTileButton(cocos2d::Ref *sender);

    int calcMeldedIdx(int maxIdx) const;
    void addFixedChowPack(mahjong::tile_t tile, int meldedIdx);
    void addFixedPungPack(mahjong::tile_t tile, int meldedIdx);
    void addFixedMeldedKongPack(mahjong::tile_t tile, int meldedIdx);
    void addFixedConcealedKongPack(mahjong::tile_t tile);
};

#endif
