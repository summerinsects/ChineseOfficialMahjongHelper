#ifndef _RECORD_SCENE_H_
#define _RECORD_SCENE_H_

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class RecordScene : public cocos2d::Layer, public cocos2d::ui::EditBoxDelegate {
public:
    static cocos2d::Scene *createScene(int index, const char **name, const std::function<void (const int (&scores)[4])> &okCallback);

    bool initWithIndex(int index, const char **name);

    virtual void onKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event *unusedEvent) override;
    virtual void editBoxReturn(cocos2d::ui::EditBox *editBox) override;

private:
    cocos2d::ui::EditBox *_editBox;
    cocos2d::ui::Button *_drawButton;
    cocos2d::Label *_nameLabel[4];
    cocos2d::ui::Button *_winButton[4];
    cocos2d::ui::Button *_selfDrawnButton[4];
    cocos2d::ui::Button *_claimButton[4];
    cocos2d::ui::Button *_falseWinButton[4];
    cocos2d::Label *_scoreLabel[4];
    cocos2d::ui::Button *_okButton;

    int _winIndex;
    int _scoreTable[4];
    std::function<void (const int (&scores)[4])> _okCallback;

    void updateScoreLabel();

    void drawCallback(cocos2d::Ref *sender);
    void winCallback(cocos2d::Ref *sender, int index);
    void selfDrawnCallback(cocos2d::Ref *sender, int index);
    void claimCallback(cocos2d::Ref *sender, int index);
    void falseWinCallback(cocos2d::Ref *sender, int index);

    void okCallback(cocos2d::Ref *sender);
};

#endif
