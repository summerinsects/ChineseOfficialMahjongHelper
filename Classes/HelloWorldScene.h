#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"

class HelloWorld : public cocos2d::Scene {
public:
    virtual bool init() override;

    CREATE_FUNC(HelloWorld);

private:
    void upgradeDataIfNecessary();
    void requestTips();
    void parseTips(std::vector<char> *buffer);
};

#endif // __HELLOWORLD_SCENE_H__
