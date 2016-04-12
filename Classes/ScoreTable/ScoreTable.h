#ifndef __SCORE_TABLE_H__
#define __SCORE_TABLE_H__

#include "../BaseLayer.h"

class ScoreTableScene : public BaseLayer {
public:
    static cocos2d::Scene *createScene();

    virtual bool init() override;

    CREATE_FUNC(ScoreTableScene);
};

#endif
