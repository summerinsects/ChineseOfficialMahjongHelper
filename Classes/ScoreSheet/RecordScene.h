#ifndef _RECORD_SCENE_H_
#define _RECORD_SCENE_H_

#include "../BaseLayer.h"
#include "Record.h"

class RecordScene : public BaseLayer, public cocos2d::ui::EditBoxDelegate {
public:
    static cocos2d::Scene *createScene(size_t handIdx, const char **playerNames, const Record::Detail &detail, const std::function<void (const Record::Detail &)> &okCallback);

    bool initWithIndex(size_t handIdx, const char **playerNames, const Record::Detail &detail);

    virtual void editBoxReturn(cocos2d::ui::EditBox *editBox) override;

    const Record::Detail &getDetail() const { return _detail; }

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
    Record::Detail _detail;
    std::function<void (const Record::Detail &)> _okCallback;

    void refresh();
    void updateScoreLabel();

    void onMinusButton(cocos2d::Ref *sender);
    void onPlusButton(cocos2d::Ref *sender);
    void onDrawButton(cocos2d::Ref *sender);
    void onWinButton(cocos2d::Ref *sender, int index);
    void onSelfDrawnButton(cocos2d::Ref *sender, int index);
    void onClaimButton(cocos2d::Ref *sender, int index);
    void onFalseWinButton(cocos2d::Ref *sender, int index);

    void onPointsNameButton(cocos2d::Ref *sender, int index);
    void onOkButton(cocos2d::Ref *sender);
};

#endif
