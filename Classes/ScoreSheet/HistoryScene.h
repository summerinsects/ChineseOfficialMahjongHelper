#ifndef __HISTORY_SCENE_H__
#define __HISTORY_SCENE_H__

#include "../BaseLayer.h"

namespace cw {
    class TableViewCell;
    class TableView;
}

struct Record;

class HistoryScene : public BaseLayer {
public:
    static cocos2d::Scene *createScene(const std::function<bool (const Record &)> &viewCallback);

    virtual bool init() override;

    CREATE_FUNC(HistoryScene);

    static void addRecord(const Record &record);
    static void modifyRecord(const Record &record);

private:
    cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx);
    void onDeleteButton(cocos2d::Ref *sender);
    void onViewButton(cocos2d::Ref *sender);

    cw::TableView *_tableView = nullptr;
    std::function<bool (const Record &)> _viewCallback;
};

#endif
