#ifndef  __LATEST_COMPETITION_SCENE_H__
#define  __LATEST_COMPETITION_SCENE_H__

#include "../BaseScene.h"
#include "../cocos-wheels/CWTableView.h"

class  LatestCompetitionScene : public BaseScene, cw::TableViewDelegate {
public:
    virtual bool init() override;

    CREATE_FUNC(LatestCompetitionScene);

private:
    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual float tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onDetailButton(cocos2d::Ref *sender);

    void requestCompetitions();
    bool parseResponse(const char *buffer, size_t size);

    cw::TableView *_tableView = nullptr;
    cocos2d::Label *_emptyLabel = nullptr;

    enum class TIME_ACCURACY {
        UNDETERMINED = 0, MONTHS, DAYS, HOURS, MINUTES
    };

    struct CompetitionInfo {
        std::string name;
        time_t startTime;
        time_t endTime;
        std::string url;
        TIME_ACCURACY timeAccuracy;
    };

    std::vector<CompetitionInfo> _competitions;
};

#endif
