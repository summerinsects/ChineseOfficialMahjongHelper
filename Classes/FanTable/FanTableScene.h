#ifndef __FAN_TABLE_SCENE_H__
#define __FAN_TABLE_SCENE_H__

#include "../BaseScene.h"
#include "../widget/CWTableView.h"

class FanTableScene : public BaseScene, cw::TableViewDelegate {
public:
    virtual bool init() override;

    CREATE_FUNC(FanTableScene);

private:
    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual cocos2d::Size tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onPointsNameButton(cocos2d::Ref *sender);
};

#endif
