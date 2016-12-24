#ifndef __SCORE_DEFINITION_H__
#define __SCORE_DEFINITION_H__

#include "../BaseLayer.h"

class ScoreDefinitionScene : public BaseLayer {
public:
    static cocos2d::Scene *createScene(size_t idx);

    bool initWithIndex(size_t idx);

private:
    void createContentView(size_t idx);
};

#endif
