#ifndef _RECORD_SCENE_H_
#define _RECORD_SCENE_H_

#include "../BaseScene.h"
#include "../cocos-wheels/CWTableView.h"
#include "Record.h"
#include "../mahjong-algorithm/fan_calculator.h"

class TilePickWidget;
class ExtraInfoWidget;

static const char *handNameText[] = {
    "东风东", "东风南", "东风西", "东风北", "南风东", "南风南", "南风西", "南风北",
    "西风东", "西风南", "西风西", "西风北", "北风东", "北风南", "北风西", "北风北"
};

#define EMOJI_FLOWER "\xF0\x9F\x8C\xB8"

#define EMOJI_FLOWER_8 EMOJI_FLOWER EMOJI_FLOWER EMOJI_FLOWER EMOJI_FLOWER EMOJI_FLOWER EMOJI_FLOWER EMOJI_FLOWER EMOJI_FLOWER

class RecordScene : public BaseScene, cocos2d::ui::EditBoxDelegate, cw::TableViewDelegate {
public:
    typedef std::function<void (const Record::Detail &)> SubmitCallback;

    CREATE_FUNC_WITH_PARAM_4(RecordScene, initWithIndex, size_t, handIdx, const char **, playerNames, const Record::Detail *, detail, const SubmitCallback &, okCallback);
    bool initWithIndex(size_t handIdx, const char **playerNames, const Record::Detail *detail, const SubmitCallback &callback);

    virtual void editBoxEditingDidBegin(cocos2d::ui::EditBox* editBox) override;
    virtual void editBoxReturn(cocos2d::ui::EditBox *editBox) override;

    const Record::Detail &getDetail() const { return _detail; }

    static void SetScoreLabelColor(cocos2d::Label *(&scoreLabel)[4], int (&scoreTable)[4], uint8_t win_claim, uint8_t false_win);

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
    cocos2d::ui::Button *_submitButton = nullptr;

    size_t _handIdx = 0;
    int _winIndex = -1;
    Record::Detail _detail;
    SubmitCallback _submitCallback;
    uint8_t _seatFlag;
    uint8_t _playerFlag;

    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual float tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void refresh();
    void updateScoreLabel();
    void showPackedFanAlert(const std::function<void ()> &callback);

    void onMinusButton(cocos2d::Ref *sender, int delta);
    void onPlusButton(cocos2d::Ref *sender, int delta);
    void onRecordTilesButton(cocos2d::Ref *sender);
    void onDrawBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onWinGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event);
    void onClaimGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event);
    void onFalseWinBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);

    void onPointsNameButton(cocos2d::Ref *sender);
    void onSubmitButton(cocos2d::Ref *sender);

    void showCalculator(const mahjong::calculate_param_t &param);
    void calculate(TilePickWidget *tilePicker, ExtraInfoWidget *extraInfo, const mahjong::calculate_param_t &param);
};

#endif
