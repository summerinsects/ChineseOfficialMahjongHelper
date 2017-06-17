#ifndef _TILE_PICK_WIDGET_H_
#define _TILE_PICK_WIDGET_H_

#include "HandTilesWidget.h"

class TilePickWidget : public cocos2d::Node {
public:
    CREATE_FUNC(TilePickWidget);

    void setFixedPacksChangedCallback(const std::function<void ()> &callback) { _fixedPacksChangedCallback = callback; }
    void setWinTileChangedCallback(const std::function<void ()> &callback) { _winTileChangedCallback = callback; }
    void setData(const mahjong::hand_tiles_t &hand_tiles, mahjong::tile_t winTile);

    virtual bool init() override;

private:
    HandTilesWidget *_handTilesWidget = nullptr;

    cocos2d::ui::Button *_characterButtons[9];
    cocos2d::ui::Button *_bambooButtons[9];
    cocos2d::ui::Button *_dotsButtons[9];
    cocos2d::ui::Button *_honorButtons[7];
    cocos2d::ui::Button *_chow_XXButton = nullptr;
    cocos2d::ui::Button *_chowX_XButton = nullptr;
    cocos2d::ui::Button *_chowXX_Button = nullptr;
    cocos2d::ui::Button *_pungButton = nullptr;
    cocos2d::ui::Button *_meldedKongButton = nullptr;
    cocos2d::ui::Button *_concealedKongButton = nullptr;

    std::function<void ()> _fixedPacksChangedCallback;
    std::function<void ()> _winTileChangedCallback;

    void reset();
    void sort();
    void refreshTilesTableButton(mahjong::tile_t tile);
    void refreshAllTilesTableButton();
    void refreshActionButtons();
    void onTileTableButton(cocos2d::Ref *sender, mahjong::tile_t tile);
    void onChow_XXButton(cocos2d::Ref *sender);
    void onChowX_XButton(cocos2d::Ref *sender);
    void onChowXX_Button(cocos2d::Ref *sender);
    void onPungButton(cocos2d::Ref *sender);
    void onMeldedKongButton(cocos2d::Ref *sender);
    void onConcealedKongButton(cocos2d::Ref *sender);

public:
    void getData(mahjong::hand_tiles_t *handTiles, mahjong::tile_t *servingTile) const {
        _handTilesWidget->getData(handTiles, servingTile);
    }

    mahjong::tile_t getServingTile() const { return _handTilesWidget->getServingTile(); }
    bool isFixedPacksContainsKong() const { return _handTilesWidget->isFixedPacksContainsKong(); }
    bool isStandingTilesContainsServingTile() const { return _handTilesWidget->isStandingTilesContainsServingTile(); }
    size_t countServingTileInFixedPacks() const { return _handTilesWidget->countServingTileInFixedPacks(); }
};

#endif
