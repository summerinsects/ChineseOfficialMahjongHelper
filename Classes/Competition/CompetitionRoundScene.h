#ifndef __COMPETITION_ROUND_SCENE__
#define __COMPETITION_ROUND_SCENE__

#include "../BaseScene.h"
#include "../widget/CWTableView.h"

class CompetitionData;
class CompetitionPlayer;

class CompetitionRoundScene : public BaseScene, cw::TableViewDelegate {
public:
    bool initWithData(const std::shared_ptr<CompetitionData> &competitionData);

    static CompetitionRoundScene *create(const std::shared_ptr<CompetitionData> &competitionData);

private:
    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual cocos2d::Size tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onRankButton(cocos2d::Ref *sender);

    float _colWidth[7];
    float _posX[7];

    std::shared_ptr<CompetitionData> _competitionData;
    std::vector<CompetitionPlayer *> _players;
};

#endif
