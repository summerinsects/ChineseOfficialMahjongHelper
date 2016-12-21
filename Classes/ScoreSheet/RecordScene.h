#ifndef _RECORD_SCENE_H_
#define _RECORD_SCENE_H_

#include "../BaseLayer.h"
#include "Record.h"

class RecordScene : public BaseLayer, public cocos2d::ui::EditBoxDelegate {
public:
    static cocos2d::Scene *createScene(size_t handIdx, const char **playerNames, const Record::Detail *detail, const std::function<void (const Record::Detail &)> &okCallback);

    bool initWithIndex(size_t handIdx, const char **playerNames, const Record::Detail *detail);

    virtual void editBoxReturn(cocos2d::ui::EditBox *editBox) override;

    const Record::Detail &getDetail() const { return _detail; }

private:
    cocos2d::ui::EditBox *_editBox = nullptr;
    cocos2d::ui::CheckBox *_drawBox = nullptr;
    cocos2d::ui::RadioButtonGroup *_winGroup = nullptr;
    cocos2d::ui::RadioButtonGroup *_claimGroup = nullptr;
    cocos2d::Label *_byDiscardLabel[4];
    cocos2d::Label *_selfDrawnLabel[4];
    cocos2d::ui::CheckBox *_falseWinBox[4];
    cocos2d::Label *_scoreLabel[4];
    cocos2d::ui::Button *_okButton = nullptr;

    int _winIndex = -1;
    Record::Detail _detail;
    std::function<void (const Record::Detail &)> _okCallback;

    void refresh();
    void updateScoreLabel();

    void onMinusButton(cocos2d::Ref *sender);
    void onPlusButton(cocos2d::Ref *sender);
    void onDrawBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onWinGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event);
    void onClaimGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event);
    void onFalseWinBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);

    void onPointsNameButton(cocos2d::Ref *sender, int index);
    void onOkButton(cocos2d::Ref *sender);
};

#endif
