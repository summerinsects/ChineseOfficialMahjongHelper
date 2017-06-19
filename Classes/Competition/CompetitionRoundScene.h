#ifndef __COMPETITION_ROUND_SCENE__
#define __COMPETITION_ROUND_SCENE__

#include "../BaseScene.h"
#include "../widget/CWTableView.h"

class CompetitionRoundScene : public BaseScene, cw::TableViewDelegate {
public:
    virtual bool init() override;

    CREATE_FUNC(CompetitionRoundScene);
    
private:
    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual cocos2d::Size tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    float _colWidth[8];
};

#endif
