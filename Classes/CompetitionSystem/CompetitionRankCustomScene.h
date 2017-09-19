#ifndef __COMPETITION_RANK_CUSTOM_SCENE__
#define __COMPETITION_RANK_CUSTOM_SCENE__

#include "../BaseScene.h"
#include "../widget/CWTableView.h"

class CompetitionData;
struct CompetitionTable;

class CompetitionRankCustomScene : public BaseScene, cw::TableViewDelegate {
public:
    CREATE_FUNC_WITH_PARAM_2(CompetitionRankCustomScene, initWithData, const std::shared_ptr<CompetitionData> &, competitionData, size_t, currentRound);
    bool initWithData(const std::shared_ptr<CompetitionData> &competitionData, size_t currentRound);

private:
    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual cocos2d::Size tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onNameWidget(cocos2d::Ref *sender);
    void showSelectPlayerAlert(size_t table, int seat);

    float _colWidth[8];
    float _posX[8];

    cw::TableView *_tableView = nullptr;
    cocos2d::ui::Button *_okButton = nullptr;

    std::shared_ptr<CompetitionData> _competitionData;
    size_t _currentRound;

    std::vector<CompetitionTable> _competitionTables;
    std::vector<uint8_t> _playerFlags;
};

#endif
