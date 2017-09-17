#ifndef __COMPETITION_ROUND_SCENE__
#define __COMPETITION_ROUND_SCENE__

#include "../BaseScene.h"
#include "../widget/CWTableView.h"

class CompetitionData;
class CompetitionPlayer;

class CompetitionRoundScene : public BaseScene, cw::TableViewDelegate {
public:
    bool initWithData(const std::shared_ptr<CompetitionData> &competitionData, unsigned currentRound);

    static CompetitionRoundScene *create(const std::shared_ptr<CompetitionData> &competitionData, unsigned currentRound);

private:
    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual cocos2d::Size tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onReportButton(cocos2d::Ref *sender);

    float _colWidth[7];
    float _posX[7];

    cw::TableView *_tableView = nullptr;

    std::shared_ptr<CompetitionData> _competitionData;
    unsigned _currentRound = 0;

    std::vector<const CompetitionPlayer *> _players;
};

#endif
