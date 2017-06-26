#ifndef  __LATEST_COMPETITION_SCENE_H__
#define  __LATEST_COMPETITION_SCENE_H__

#include "../BaseScene.h"
#include "../widget/CWTableView.h"

class  LatestCompetitionScene : public BaseScene, cw::TableViewDelegate {
public:
    virtual bool init() override;

    CREATE_FUNC(LatestCompetitionScene);

private:
    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual cocos2d::Size tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onDetailButton(cocos2d::Ref *sender);

    cw::TableView *_tableView = nullptr;

    struct CompetitionInfo {
        std::string name;
        time_t startTime;
        time_t endTime;
        std::string url;
    };

    std::vector<CompetitionInfo> _competitions;
};

#endif
