#ifndef __HISTORY_SCENE_H__
#define __HISTORY_SCENE_H__

#include "../BaseScene.h"
#include "../cocos-wheels/CWTableView.h"

struct Record;

struct RecordTexts {
    const Record *source;
    const char *title;
    std::string time;
    std::string players[4];
};

class RecordHistoryScene : public BaseScene, cw::TableViewDelegate {
public:
    typedef std::function<void (Record *)> ViewCallback;

    CREATE_FUNC_WITH_PARAM_1(RecordHistoryScene, initWithCallback, ViewCallback &&, viewCallback);
    bool initWithCallback(ViewCallback &&viewCallback);

    static void modifyRecord(const Record *record);

    virtual void onEnter() override;

private:
    std::vector<RecordTexts> _recordTexts;

    void refresh();
    void updateRecordTexts();

    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual float tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onMoreButton(cocos2d::Ref *sender);
    void onSummaryButton();
    void onBatchDeleteButton();

    void onDeleteButton(cocos2d::Ref *sender);
    void onCellClicked(cocos2d::Ref *sender);

    void saveRecordsAndRefresh();

    cocos2d::Label *_emptyLabel = nullptr;
    cw::TableView *_tableView = nullptr;
    ViewCallback _viewCallback;
};

#endif
