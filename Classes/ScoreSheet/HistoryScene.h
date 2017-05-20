#ifndef __HISTORY_SCENE_H__
#define __HISTORY_SCENE_H__

#include "../BaseLayer.h"
#include "../widget/CWTableView.h"

struct Record;

class HistoryScene : public BaseLayer, cw::TableViewDelegate {
public:
    static cocos2d::Scene *createScene(const std::function<void (Record *)> &viewCallback);

    virtual bool init() override;

    CREATE_FUNC(HistoryScene);

    static void modifyRecord(const Record *record);

private:
    struct RecordText {
        char startTime[32];
        char endTime[32];
        char score[255];
    };

    std::vector<RecordText> _recordTexts;

    void updateRecordTexts();

    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual cocos2d::Size tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onDeleteButton(cocos2d::Ref *sender);
    void onCellEvent(cocos2d::Ref *sender, cocos2d::ui::Widget::TouchEventType event);

    cw::TableView *_tableView = nullptr;
    std::function<void (Record *)> _viewCallback;
};

#endif
