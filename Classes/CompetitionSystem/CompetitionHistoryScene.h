#ifndef __COMPETITION_HISTORY_SCENE_H__
#define __COMPETITION_HISTORY_SCENE_H__

#include "../BaseScene.h"
#include "../cocos-wheels/CWTableView.h"

class CompetitionData;

class CompetitionHistoryScene : public BaseScene, cw::TableViewDelegate {
public:
    typedef std::function<void (CompetitionData *)> ViewCallback;

    CREATE_FUNC_WITH_PARAM_1(CompetitionHistoryScene, initWithCallback, const ViewCallback &, viewCallback);
    bool initWithCallback(const ViewCallback &viewCallback);

    static void modifyData(const CompetitionData *data);

private:
    std::vector<std::string> _dataTexts;

    void updateDataTexts();

    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual float tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onDeleteButton(cocos2d::Ref *sender);
    void onCellClicked(cocos2d::Ref *sender);

    cw::TableView *_tableView = nullptr;
    ViewCallback _viewCallback;
};

#endif
