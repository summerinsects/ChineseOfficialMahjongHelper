#ifndef _RECORD_SCENE_H_
#define _RECORD_SCENE_H_

#include "../BaseLayer.h"
#include "../widget/CWTableView.h"
#include "Record.h"
#include "../mahjong-algorithm/fan_calculator.h"

#define SET_FAN(flag_, fan_) ((flag_) |= (1ULL << (mahjong::LAST_TILE - (fan_))))
#define RESET_FAN(flag_, fan_) ((flag_) &= ~(1ULL << (mahjong::LAST_TILE - (fan_))))
#define TEST_FAN(flag_, fan_) !!((flag_) & (1ULL << (mahjong::LAST_TILE - (fan_))))

class TilePickWidget;
class ExtraInfoWidget;

class RecordScene : public BaseLayer, cocos2d::ui::EditBoxDelegate, cw::TableViewDelegate {
public:
    static cocos2d::Scene *createScene(size_t handIdx, const char **playerNames, const Record::Detail *detail, const std::function<void (const Record::Detail &)> &okCallback);

    bool initWithIndex(size_t handIdx, const char **playerNames, const Record::Detail *detail);

    virtual void editBoxReturn(cocos2d::ui::EditBox *editBox) override;

    const Record::Detail &getDetail() const { return _detail; }

    typedef struct {
        mahjong::hand_tiles_t hand_tiles;
        mahjong::tile_t win_tile;
        int flower_count;
        mahjong::extra_condition_t ext_cond;
    } CalculateParam;

    static void _WinHandToCalculateParam(const Record::Detail::WinHand &winHand, CalculateParam &param);
    static void _CalculateParamToWinHand(const CalculateParam &param, Record::Detail::WinHand &winHand);

private:
    cocos2d::ui::EditBox *_editBox = nullptr;
    cocos2d::ui::CheckBox *_drawBox = nullptr;
    cocos2d::ui::RadioButtonGroup *_winGroup = nullptr;
    cocos2d::ui::RadioButtonGroup *_claimGroup = nullptr;
    cocos2d::Label *_byDiscardLabel[4];
    cocos2d::Label *_selfDrawnLabel[4];
    cocos2d::ui::CheckBox *_falseWinBox[4];
    cocos2d::Label *_scoreLabel[4];
    cw::TableView *_tableView;
    cocos2d::ui::Button *_okButton = nullptr;

    size_t _handIdx = 0;
    int _winIndex = -1;
    Record::Detail _detail;
    std::function<void (const Record::Detail &)> _okCallback;

    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual cocos2d::Size tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void refresh();
    void updateScoreLabel();

    void onMinusButton(cocos2d::Ref *sender);
    void onPlusButton(cocos2d::Ref *sender);
    void onTilesButton(cocos2d::Ref *sender);
    void onDrawBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);
    void onWinGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event);
    void onClaimGroup(cocos2d::ui::RadioButton *radioButton, int index, cocos2d::ui::RadioButtonGroup::EventType event);
    void onFalseWinBox(cocos2d::Ref *sender, cocos2d::ui::CheckBox::EventType event);

    void onPointsNameButton(cocos2d::Ref *sender);
    void onOkButton(cocos2d::Ref *sender);

    void showCalculator(const CalculateParam &param);
    void calculate(TilePickWidget *tilePicker, ExtraInfoWidget *extraInfo, const CalculateParam &param);
};

#endif
