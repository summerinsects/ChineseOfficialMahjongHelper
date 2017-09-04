#ifndef __TILES_KEYBOARD_H__
#define __TILES_KEYBOARD_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "../mahjong-algorithm/tile.h"

#define INPUT_GUIDE_STRING_1 "数牌：万=m 条=s 饼=p。后缀使用小写字母，一连串同花色的数牌可合并使用用一个后缀，如123m、678s等等。"
#define INPUT_GUIDE_STRING_2 "字牌：东南西北=ESWN，中发白=CFP。使用大写字母。亦兼容天凤风格的后缀z，但按中国习惯顺序567z为中发白。"
#define INPUT_GUIDE_STRING_3 "每组吃、碰、明杠之间用英文空格分隔，每一组暗杠前后用英文[]。副露与立牌之间也用英文空格分隔。"

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

    void refreshSuit();
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
    cocos2d::ui::Button *_digitButton[9];
    int _currentSuit = 0;
    bool _inBracket = false;

    std::function<void (const char *)> _onTextChange;
    std::function<void (DismissEvent)> _onDismiss;
};

#endif
