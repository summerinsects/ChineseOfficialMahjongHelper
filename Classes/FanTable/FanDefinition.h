#ifndef __FAN_DEFINITION_H__
#define __FAN_DEFINITION_H__

#include "../BaseScene.h"

class FanDefinitionScene : public BaseScene {
public:
    CREATE_FUNC_WITH_PARAM_1(FanDefinitionScene, initWithIndex, size_t, idx);

    bool initWithIndex(size_t idx);

private:
    void createContentView(size_t idx);
};

#endif
