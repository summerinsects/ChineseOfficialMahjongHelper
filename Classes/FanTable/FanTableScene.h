#ifndef __FAN_TABLE_SCENE_H__
#define __FAN_TABLE_SCENE_H__

#include "../BaseScene.h"
#include "../cocos-wheels/CWTableView.h"

class FanTableScene : public BaseScene, cw::TableViewDelegate {
public:
    virtual bool init() override;

    CREATE_FUNC(FanTableScene);

    static void asyncShowFanDefinition(size_t idx);

private:
    virtual ssize_t numberOfCellsInTableView(cw::TableView *table) override;
    virtual float tableCellSizeForIndex(cw::TableView *table, ssize_t idx) override;
    virtual cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx) override;

    void onFanNameButton(cocos2d::Ref *sender);
};

#endif
