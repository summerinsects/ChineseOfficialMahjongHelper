#ifndef __COMPETITION_ENTER_SCENE__
#define __COMPETITION_ENTER_SCENE__

#include "../BaseScene.h"
#include "../widget/CWTableView.h"

class CompetitionData;

class CompetitionEnterScene : public BaseScene, cw::TableViewDelegate {
public:
    static CompetitionEnterScene *create(const std::shared_ptr<CompetitionData> &competitionData);
    bool initWithData(const std::shared_ptr<CompetitionData> &competitionData);

private:
    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual cocos2d::Size tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onOkButton(cocos2d::Ref *sender);
    void onNameButton(cocos2d::Ref *sender);

    cw::TableView *_tableView = nullptr;

    std::shared_ptr<CompetitionData> _competitionData;
};

#endif

