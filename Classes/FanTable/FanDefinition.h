#ifndef __FAN_DEFINITION_H__
#define __FAN_DEFINITION_H__

#include "../BaseLayer.h"

#pragma execution_character_set("utf-8")

static const char *principle_title[] = { "不重复原则", "不拆移原则", "不得相同原则", "就高不就低原则", "套算一次原则" };

class FanDefinitionScene : public BaseLayer {
public:
    static cocos2d::Scene *createScene(size_t idx);

    bool initWithIndex(size_t idx);

private:
    void createContentView(size_t idx);
};

#endif
