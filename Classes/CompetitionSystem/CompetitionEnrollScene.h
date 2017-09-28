#ifndef __COMPETITION_ENROLL_SCENE__
#define __COMPETITION_ENROLL_SCENE__

#include "../BaseScene.h"
#include "../cocos-wheels/CWTableView.h"

class CompetitionData;

class CompetitionEnrollScene : public BaseScene, cw::TableViewDelegate {
public:
    CREATE_FUNC_WITH_PARAM_1(CompetitionEnrollScene, initWithData, const std::shared_ptr<CompetitionData> &, competitionData);
    bool initWithData(const std::shared_ptr<CompetitionData> &competitionData);

private:
    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual float tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onSubmitButton(cocos2d::Ref *sender);
    void onNameWidget(cocos2d::Ref *sender);

    cw::TableView *_tableView = nullptr;

    std::shared_ptr<CompetitionData> _competitionData;
};

#endif

