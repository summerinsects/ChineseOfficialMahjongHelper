#ifndef _TILE_PICK_WIDGET_H_
#define _TILE_PICK_WIDGET_H_

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "../mahjong-algorithm/points_calculator.h"

class TilePickWidget : public cocos2d::ui::Widget {
public:
    CREATE_FUNC(TilePickWidget);

    const std::vector<mahjong::TILE> &getStandingTiles() const { return _standingTiles; }
    const std::vector<mahjong::SET> &getFixedSets() const { return _fixedSets; }
    mahjong::TILE getWinTile() const { return _winTile; }

    void setFixedSetsChangedCallback(const std::function<void (TilePickWidget *)> &callback) {
        _fixedSetsChangedCallback = callback;
    }

    void setWinTileChangedCallback(const std::function<void (TilePickWidget *)> &callback) {
        _winTileChangedCallback = callback;
    }

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
    cocos2d::ui::Widget *_standingWidget;
    std::vector<cocos2d::ui::Button *> _standingTileButtons;
    cocos2d::ui::Button *_winTileButton;
    cocos2d::DrawNode *_highlightBox;

    int _totalTilesTable[0x54];
    int _standingTilesTable[0x54];
    std::vector<mahjong::TILE> _standingTiles;
    std::vector<mahjong::SET> _fixedSets;
    mahjong::TILE _winTile;
    size_t _currentIdx;

    std::function<void (TilePickWidget *)> _fixedSetsChangedCallback;
    std::function<void (TilePickWidget *)> _winTileChangedCallback;

    void reset();
    void sort();
    cocos2d::Vec2 calcStandingTilePos(size_t idx) const;
    void addOneTile(mahjong::TILE tile, bool isWinTile);
    void replaceOneTile(mahjong::TILE tile, bool isWinTile);
    void refreshTilesTableButton(mahjong::TILE tile);
    void onTileTableButton(cocos2d::Ref *sender, mahjong::TILE tile);
    void refreshActionButtons();
    void refreshAfterAction(int meldedIdx);
    void refreshStandingTiles();
    void addFixedChowSet(const cocos2d::Vec2 &center, mahjong::TILE tile, int meldedIdx);
    void addFixedPungSet(const cocos2d::Vec2 &center, mahjong::TILE tile, int meldedIdx);
    void addFixedMeldedKongSet(const cocos2d::Vec2 &center, mahjong::TILE tile, int meldedIdx);
    void addFixedConcealedKongSet(const cocos2d::Vec2 &center, mahjong::TILE tile);
    void onChowLessButton(cocos2d::Ref *sender);
    void onChowMidButton(cocos2d::Ref *sender);
    void onChowGreatButton(cocos2d::Ref *sender);
    void onPungButton(cocos2d::Ref *sender);
    void onMeldedKongButton(cocos2d::Ref *sender);
    void onConcealedKongButton(cocos2d::Ref *sender);
};

#endif
