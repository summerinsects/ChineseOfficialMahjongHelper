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

    typedef std::function<void (const CompetitionResult &, bool)> RefreshRecordAlertCallback;

    void onClearButton(cocos2d::Ref *sender);
    void onRecordButton(cocos2d::Ref *sender);
    void showRecordAlert(size_t table, const CompetitionResult (&prevResult)[4]);
    void showArrangeAlert();

    float _colWidth[7];
    float _posX[7];

    cw::TableView *_tableView = nullptr;
    cocos2d::ui::Button *_submitButton = nullptr;

    std::shared_ptr<CompetitionData> _competitionData;
    size_t _currentRound;

    std::vector<CompetitionTable> *_competitionTables;
};

#endif
