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

    CREATE_FUNC_WITH_PARAM_1(RecordHistoryScene, initWithCallback, const ViewCallback &, viewCallback);
    bool initWithCallback(const ViewCallback &viewCallback);

    static void modifyRecord(const Record *record);

private:
    std::vector<RecordTexts> _recordTexts;

    void updateRecordTexts();

    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual float tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onSummaryButton(cocos2d::Ref *sender);
    void onBatchDeleteButton(cocos2d::Ref *sender);

    void onDeleteButton(cocos2d::Ref *sender);
    void onCellClicked(cocos2d::Ref *sender);

    cw::TableView *_tableView = nullptr;
    ViewCallback _viewCallback;
};

#endif
