#ifndef __COMPETITION_TABLE_SCENE_H__
#define __COMPETITION_TABLE_SCENE_H__

#include "../BaseScene.h"
#include "../cocos-wheels/CWTableView.h"

class CompetitionData;
struct CompetitionResult;
struct CompetitionTable;

class CompetitionTableScene : public BaseScene, cw::TableViewDelegate {
public:
    CREATE_FUNC_WITH_PARAM_2(CompetitionTableScene, initWithData, const std::shared_ptr<CompetitionData> &, competitionData, size_t, currentRound);
    bool initWithData(const std::shared_ptr<CompetitionData> &competitionData, size_t currentRound);

private:
    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual float tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onClearButton(cocos2d::Ref *sender);
    void onRecordButton(cocos2d::Ref *sender);
    void showRecordAlert(size_t table);
    void showCompetitionResultInputAlert(const std::string &title, CompetitionResult *result, const std::function<void ()> &callback);
    void showRankAlert();

    float _colWidth[8];
    float _posX[8];

    cw::TableView *_tableView = nullptr;
    cocos2d::ui::Button *_submitButton = nullptr;

    std::shared_ptr<CompetitionData> _competitionData;
    size_t _currentRound;

    std::vector<CompetitionTable> *_competitionTables;
};

#endif
