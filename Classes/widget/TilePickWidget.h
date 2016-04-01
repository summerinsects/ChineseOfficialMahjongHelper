#ifndef _TILE_PICK_WIDGET_H_
#define _TILE_PICK_WIDGET_H_

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "../mahjong-algorithm/points_calculator.h"

class TilePickWidget : public cocos2d::ui::Widget {
public:
    CREATE_FUNC(TilePickWidget);

    const std::vector<TILE> &getHandTiles() const { return _tiles; }
    const std::vector<SET> &getFixedSets() const { return _fixedSets; }
    TILE getWinTile() const { return _winTile; }

CC_CONSTRUCTOR_ACCESS:
    virtual bool init() override;

private:
    cocos2d::ui::Button *_characterButtons[9];
    cocos2d::ui::Button *_bambooButtons[9];
    cocos2d::ui::Button *_dotsButtons[9];
    cocos2d::ui::Button *_honorButtons[7];
    cocos2d::ui::Button *_chowLessButton;
    cocos2d::ui::Button *_chowMidButton;
    cocos2d::ui::Button *_chowGreatButton;
    cocos2d::ui::Button *_pungButton;
    cocos2d::ui::Button *_meldedKongButton;
    cocos2d::ui::Button *_concealedKongButton;

    cocos2d::ui::Widget *_fixedWidget;
    cocos2d::ui::Widget *_tilesWidget;
    std::vector<cocos2d::ui::Button *> _tileButtons;
    cocos2d::ui::Button *_winTileButton;
    cocos2d::DrawNode *_highlightBox;

    int _totalTilesTable[0x54];
    int _handTilesTable[0x54];
    std::vector<TILE> _tiles;
    std::vector<SET> _fixedSets;
    TILE _winTile;
    size_t _currentIdx;

    void reset();
    void sort();
    cocos2d::Vec2 calcHandTilePos(size_t idx) const;
    void addOneTile(TILE tile, bool isWinTile);
    void tileTableCallback(cocos2d::Ref *sender, TILE tile);
    void refreshActionButtons();
    void refreshAfterAction(int meldedIdx);
    void refreshHandTiles();
    void addFixedChowSet(const cocos2d::Vec2 &center, TILE tile, int meldedIdx);
    void addFixedPungSet(const cocos2d::Vec2 &center, TILE tile, int meldedIdx);
    void addFixedMeldedKongSet(const cocos2d::Vec2 &center, TILE tile, int meldedIdx);
    void addFixedConcealedKongSet(const cocos2d::Vec2 &center, TILE tile);
    void chowLessCallback(cocos2d::Ref *sender);
    void chowMidCallback(cocos2d::Ref *sender);
    void chowGreatCallback(cocos2d::Ref *sender);
    void pungCallback(cocos2d::Ref *sender);
    void meldedKongCallback(cocos2d::Ref *sender);
    void concealedKongCallback(cocos2d::Ref *sender);
};

#endif
