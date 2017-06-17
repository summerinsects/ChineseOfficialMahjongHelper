#ifndef __OTHER_SCENE_H__
#define __OTHER_SCENE_H__

#include "../BaseScene.h"

class OtherScene : public BaseScene {
public:
    virtual bool init() override;

    CREATE_FUNC(OtherScene);

private:
    void createContentView();
};

#endif
