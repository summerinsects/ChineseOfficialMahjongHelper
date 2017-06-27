#ifndef __COMPETITION_ENTER_SCENE__
#define __COMPETITION_ENTER_SCENE__

#include "../BaseScene.h"
#include "../widget/CWTableView.h"

class CompetitionEnterScene : public BaseScene, cw::TableViewDelegate {
public:
    static CompetitionEnterScene *create(const std::string &name, unsigned num);
    bool initWithName(const std::string &name, unsigned num);

private:
    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual cocos2d::Size tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onOkButton(cocos2d::Ref *sender);
    void onNameButton(cocos2d::Ref *sender);

    std::vector<std::string> _names;
    cw::TableView *_tableView = nullptr;
};

#endif

