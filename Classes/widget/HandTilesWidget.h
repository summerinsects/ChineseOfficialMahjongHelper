#ifndef __HAND_TILES_WIDGETS_H__
#define __HAND_TILES_WIDGETS_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "../mahjong-algorithm/points_calculator.h"

class HandTilesWidget : public cocos2d::ui::Widget {
public:
    CREATE_FUNC(HandTilesWidget);

    const std::vector<mahjong::TILE> &getStandingTiles() const { return _standingTiles; }
    const std::vector<mahjong::SET> &getFixedSets() const { return _fixedSets; }

    void setCurrentIdxChangedCallback(const std::function<void ()> &callback) { _currentIdxChangedCallback = callback; }

    size_t getCurrentIdx() const { return _currentIdx; }
    int getUsedTileCount(mahjong::TILE tile) const { return _usedTilesTable[tile]; }
    int getStandingTileCount(mahjong::TILE tile) const { return _standingTilesTable[tile]; }

    void setData(const mahjong::SET fixedSets[5], long setCnt, const mahjong::TILE standingTiles[13], long tileCnt, mahjong::TILE winTile);

    mahjong::TILE getWinTile() const;
    bool isFixedSetsContainsKong() const;
    bool isStandingTilesContainsWinTile() const;
    bool isFixedSetsContainsWinTile() const;

CC_CONSTRUCTOR_ACCESS:
    virtual bool init() override;

public:
    void reset();
    void sort();
    mahjong::TILE putTile(mahjong::TILE tile);

    bool canChow_XX();
    bool canChowX_X();
    bool canChowXX_();
    bool canPung();
    bool canKong();

    bool makeFixedChow_XXSet();
    bool makeFixedChowX_XSet();
    bool makeFixedChowXX_Set();
    bool makeFixedPungSet();
    bool makeFixedMeldedKongSet();
    bool makeFixedConcealedKongSet();

private:
    cocos2d::ui::Widget *_fixedWidget;
    cocos2d::ui::Widget *_standingWidget;
    std::vector<cocos2d::ui::Button *> _standingTileButtons;
    cocos2d::DrawNode *_highlightBox;

    int _usedTilesTable[0x54];
    int _standingTilesTable[0x54];
    std::vector<mahjong::TILE> _standingTiles;
    std::vector<mahjong::SET> _fixedSets;
    size_t _currentIdx;

    std::function<void ()> _currentIdxChangedCallback;

private:
    cocos2d::Vec2 calcStandingTilePos(size_t idx) const;
    cocos2d::Vec2 calcFixedSetPos(size_t idx) const;

    void addTile(mahjong::TILE tile);
    void replaceTile(mahjong::TILE tile);
    void refreshHighlightPos();
    void refreshStandingTiles();

    void onTileButton(cocos2d::Ref *sender);

    void addFixedChowSet(mahjong::TILE tile, int meldedIdx);
    void addFixedPungSet(mahjong::TILE tile, int meldedIdx);
    void addFixedMeldedKongSet(mahjong::TILE tile, int meldedIdx);
    void addFixedConcealedKongSet(mahjong::TILE tile);
};

#endif
