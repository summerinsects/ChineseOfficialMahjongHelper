#ifndef __OTHER_SCENE_H__
#define __OTHER_SCENE_H__

#include "../BaseLayer.h"

class OtherScene : public BaseLayer {
public:
    static cocos2d::Scene *createScene();

    virtual bool init() override;

    CREATE_FUNC(OtherScene);

private:
    void createContentView();
};

#endif
