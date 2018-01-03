#ifndef _TOAST_H_
#define _TOAST_H_

#include "cocos2d.h"

class Toast : public cocos2d::Node {
public:
    static inline float maxWidth() {
        return cocos2d::Director::getInstance()->getVisibleSize().width * 0.8f - 10.0f;
    }

    enum Duration { LENGTH_SHORT, LENGTH_LONG };

    bool initWithText(cocos2d::Scene *scene, const std::string &text, Duration duration);

    static Toast *makeText(cocos2d::Scene *scene, const std::string &text, Duration duration);

    void show();

private:
    cocos2d::RefPtr<cocos2d::Scene> _scene;
    Duration _duration;
};

#endif
