#ifndef __TILES_KEYBOARD_H__
#define __TILES_KEYBOARD_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "../mahjong-algorithm/tile.h"

class TilesKeyboard : public cocos2d::Layer {
public:
    CREATE_FUNC(TilesKeyboard);

CC_CONSTRUCTOR_ACCESS:
    virtual bool init() override;

private:
    void onKeyboardButton(cocos2d::Ref *sender);

    void addTiles(const mahjong::tile_t *tiles, size_t count);
    void removeTiles(size_t count);
    void addSpaceTile();
    void removeSpaceTile();

    void onNumberedSuffix(int suit);
    void onHonor(int honor);
    void onBackspace();
    void onSpace();
    void onLeftBracket();
    void onRightBracket();

    cocos2d::Label *_textLabel = nullptr;
    cocos2d::Node *_tilesContainer = nullptr;
    std::vector<cocos2d::Sprite *> _tilesSprite;
    bool _inBracket = false;
};

#endif
