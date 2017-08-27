#ifndef _RECORD_SCENE_H_
#define _RECORD_SCENE_H_

#include "../BaseScene.h"
#include "../widget/CWTableView.h"
#include "Record.h"
#include "../mahjong-algorithm/fan_calculator.h"

static const char *handNameText[] = {
    "东风东", "东风南", "东风西", "东风北", "南风东", "南风南", "南风西", "南风北",
    "西风东", "西风南", "西风西", "西风北", "北风东", "北风南", "北风西", "北风北"
};

class RecordScene : public BaseScene, cocos2d::ui::EditBoxDelegate, cw::TableViewDelegate {
public:
    static cocos2d::Scene *create(size_t handIdx, const char **playerNames, const Record::Detail *detail, const std::function<void (const Record::Detail &)> &okCallback);

    bool initWithIndex(size_t handIdx, const char **playerNames, const Record::Detail *detail);

    virtual void editBoxReturn(cocos2d::ui::EditBox *editBox) override;

    const Record::Detail &getDetail() const { return _detail; }

    static void _SetScoreLabelColor(cocos2d::Label *(&scoreLabel)[4], int (&scoreTable)[4], uint8_t win_claim, uint8_t false_win);

private:
    cocos2d::ui::EditBox *_editBox = nullptr;
    cocos2d::ui::CheckBox *_drawBox = nullptr;
    cocos2d::ui::RadioButtonGroup *_winGroup = nullptr;
    cocos2d::ui::RadioButtonGroup *_claimGroup = nullptr;
    cocos2d::Label *_byDiscardLabel[4];
    cocos2d::Label *_selfDrawnLabel[4];
    cocos2d::ui::CheckBox *_falseWinBox[4];
    cocos2d::Label *_scoreLabel[4];
    cw::TableView *_tableView = nullptr;
    cocos2d::ui::Button *_okButton = nullptr;

    int _winIndex = -1;
    Record::Detail _detail;
    std::function<void (const Record::Detail &)> _okCallback;

    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual cocos2d::Size tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void refresh();
    void updateScoreLabel();

    void onMinusButton(cocos2d::Ref *sender, int delta);
    void onPlusButton(cocos2d::Ref *sender, int delta);
    void onDrawBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onWinGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event);
    void onClaimGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event);
    void onFalseWinBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);

    void onPackedFanButton(cocos2d::Ref *sender);
    void onPointsNameButton(cocos2d::Ref *sender);
    void onOkButton(cocos2d::Ref *sender);
};

#endif
