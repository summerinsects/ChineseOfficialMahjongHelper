#ifndef __COMPETITION_ROUND_SCENE_H__
#define __COMPETITION_ROUND_SCENE_H__

#include "../BaseScene.h"
#include "../cocos-wheels/CWTableView.h"

class CompetitionData;
class CompetitionPlayer;

class CompetitionRoundScene : public BaseScene, cw::TableViewDelegate {
public:
    CREATE_FUNC_WITH_PARAM_2(CompetitionRoundScene, initWithData, const std::shared_ptr<CompetitionData> &, competitionData, size_t, currentRound);
    bool initWithData(const std::shared_ptr<CompetitionData> &competitionData, size_t currentRound);

private:
    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual float tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onReportButton(cocos2d::Ref *sender);

    float _colWidth[7];
    float _posX[7];

    cw::TableView *_tableView = nullptr;

    std::shared_ptr<CompetitionData> _competitionData;
    size_t _currentRound = 0;

    std::vector<const CompetitionPlayer *> _players;
};

#endif
