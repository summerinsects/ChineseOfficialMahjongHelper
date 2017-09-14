#ifndef __COMPETITION_TABLE_SCENE__
#define __COMPETITION_TABLE_SCENE__

#include "../BaseScene.h"
#include "../widget/CWTableView.h"

class CompetitionData;
struct CompetitionTable;

class CompetitionTableScene : public BaseScene, cw::TableViewDelegate {
public:
    bool initWithData(const std::shared_ptr<CompetitionData> &competitionData, unsigned currentRound);

    static CompetitionTableScene *create(const std::shared_ptr<CompetitionData> &competitionData, unsigned currentRound);

private:
    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual cocos2d::Size tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onRankButton(cocos2d::Ref *sender);
    void onRecordButton(cocos2d::Ref *sender);

    void rankBySerial();
    void rankByRandom();
    void rankByScores();
    void rankBySerialSnake();
    void rankByScoresSnake();

    float _colWidth[7];
    float _posX[7];

    cw::TableView *_tableView = nullptr;

    std::shared_ptr<CompetitionData> _competitionData;
    unsigned _currentRound;

    std::vector<CompetitionTable> *_competitionTables;
};

#endif
