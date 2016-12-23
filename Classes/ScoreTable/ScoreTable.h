#ifndef __SCORE_TABLE_H__
#define __SCORE_TABLE_H__

#include "../BaseLayer.h"

namespace cw {
    class TableViewCell;
    class TableView;
}

class ScoreTableScene : public BaseLayer {
public:
    static cocos2d::Scene *createScene();

    virtual bool init() override;

    CREATE_FUNC(ScoreTableScene);

private:
    cocos2d::Size tableCellSizeForIndex(cw::TableView *table, ssize_t idx);
    cw::TableViewCell *tableCellAtIndex(cw::TableView *table, ssize_t idx);
    void onPointsNameButton(cocos2d::Ref *sender);
};

#endif
