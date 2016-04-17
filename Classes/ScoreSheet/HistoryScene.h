#ifndef __HISTORY_SCENE_H__
#define __HISTORY_SCENE_H__

#include "../BaseLayer.h"

struct Record;

class HistoryScene : public BaseLayer {
public:
    static cocos2d::Scene *createScene();

    virtual bool init() override;

    CREATE_FUNC(HistoryScene);

    static void addRecord(const Record &record);
};

#endif
