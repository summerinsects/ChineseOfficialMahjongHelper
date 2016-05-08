#ifndef __HISTORY_SCENE_H__
#define __HISTORY_SCENE_H__

#include "../BaseLayer.h"

struct Record;

class HistoryScene : public BaseLayer {
public:
    static cocos2d::Scene *createScene(const std::function<void (const Record &)> &viewCallback);

    virtual bool init() override;

    CREATE_FUNC(HistoryScene);

    static void addRecord(const Record &record);

private:
    cocos2d::ui::Widget *createRecordWidget(size_t idx, float width);

    std::function<void (const Record &)> _viewCallback;
};

#endif
