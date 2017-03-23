#ifndef _ALERT_LAYER_H_
#define _ALERT_LAYER_H_

#include "cocos2d.h"

class AlertView : public cocos2d::Layer {
public:
    bool initWithTitle(const std::string &title, cocos2d::Node *node, const std::function<void (AlertView *)> &confirmCallback, const std::function<void (AlertView *)> &cancelCallback);

    static void showWithNode(const std::string &title, cocos2d::Node *node, const std::function<void (AlertView *)> &confirmCallback, const std::function<void (AlertView *)> &cancelCallback);
    static void showWithMessage(const std::string &title, const std::string &message, const std::function<void (AlertView *)> &confirmCallback, const std::function<void (AlertView *)> &cancelCallback);

private:
    void onCancelButton(cocos2d::Ref *sender);
    void onConfirmButton(cocos2d::Ref *sender);

    std::function<void (AlertView *)> _confirmCallback;
    std::function<void (AlertView *)> _cancelCallback;
};

#endif
