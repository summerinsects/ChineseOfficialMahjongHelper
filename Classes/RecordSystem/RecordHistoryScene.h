#ifndef __HISTORY_SCENE_H__
#define __HISTORY_SCENE_H__

#include "../BaseScene.h"
#include "../cocos-wheels/CWTableView.h"

struct Record;

class RecordHistoryScene : public BaseScene, cw::TableViewDelegate {
public:
    typedef std::function<void (Record *)> ViewCallback;

    CREATE_FUNC_WITH_PARAM_1(RecordHistoryScene, initWithCallback, const ViewCallback &, viewCallback);
    bool initWithCallback(const ViewCallback &viewCallback);

    static void modifyRecord(const Record *record);

private:
    std::vector<std::string> _recordTexts;

    void updateRecordTexts();

    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual float tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onDeleteButton(cocos2d::Ref *sender);
    void onCellClicked(cocos2d::Ref *sender);

    cw::TableView *_tableView = nullptr;
    ViewCallback _viewCallback;
};

#endif
