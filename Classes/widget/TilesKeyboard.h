#ifndef __TILES_KEYBOARD_H__
#define __TILES_KEYBOARD_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "../mahjong-algorithm/tile.h"

class TilesKeyboard : public cocos2d::Layer {
public:
    static void hookEditBox(cocos2d::ui::EditBox *editBox);
    static void showByEditBox(cocos2d::ui::EditBox *editBox);
    static const char *parseInput(const char *input, const std::function<void (const mahjong::hand_tiles_t &, mahjong::tile_t)> &callback);

    CREATE_FUNC(TilesKeyboard);

CC_CONSTRUCTOR_ACCESS:
    virtual bool init() override;

private:
    enum class DismissEvent {
        OK, CANCEL, GLOBAL
    };

    void associateWithEditBox(cocos2d::ui::EditBox *editBox);

    void onKeyboardButton(cocos2d::Ref *sender);

    void refreshInputLabel();
    void setTilesText(const char *text);

    void addTiles(const mahjong::tile_t *tiles, size_t count);
    void removeTiles(size_t count);
    void addSpaceTile();
    void removeSpaceTile();

    void dismissWithEvent(DismissEvent event);
    void onNumberedSuffix(int suit);
    void onHonor(int honor);
    void onBackspace();
    void onSpace();
    void onLeftBracket();
    void onRightBracket();
    void onClear();

    std::string _tilesText;

    cocos2d::Node *_rootNode = nullptr;
    cocos2d::Label *_inputLabel = nullptr;
    cocos2d::Label *_countLabel = nullptr;
    cocos2d::LayerColor *_inputBg = nullptr;
    cocos2d::Node *_tilesContainer = nullptr;
    std::vector<cocos2d::Sprite *> _tilesSprite;

    cocos2d::ui::Button *_suitButton = nullptr;
    int _currentSuit = 0;
    bool _inBracket = false;

    std::function<void (const char *)> _onTextChange;
    std::function<void (DismissEvent)> _onDismiss;
};

#endif
