#ifndef _ALERT_DIALOG_H_
#define _ALERT_DIALOG_H_

#include "cocos2d.h"
#include "ui/CocosGUI.h"

class AlertDialog : public cocos2d::Layer {
public:
    static inline float maxWidth() {
        return cocos2d::Director::getInstance()->getVisibleSize().width * 0.8f - 10.0f;
    }

    enum {
        BUTTON_NEGATIVE = -2,
        BUTTON_POSITIVE = -1
    };

    typedef std::function<bool (AlertDialog *, int which)> ButtonCallback;

    class Builder {
        friend class AlertDialog;

        cocos2d::RefPtr<cocos2d::Scene> _scene;
        std::string _title;
        std::string _message;
        cocos2d::RefPtr<cocos2d::Node> _contentNode;
        std::string _positiveTitle;
        ButtonCallback _positiveCallback;
        std::string _negativeTitle;
        ButtonCallback _negativeCallback;
        bool _isCancelable = true;
        bool _isCloseOnTouchOutside = true;

    public:
        explicit Builder(cocos2d::Scene *scene) : _scene(scene) { }

        Builder &setTitle(std::string &&title) { _title.swap(title); return *this; }
        Builder &setMessage(std::string &&message) { _message.swap(message); return *this; }
        Builder &setContentNode(cocos2d::Node *node) { _contentNode = node; return *this; }

        Builder &setPositiveButton(std::string &&title, ButtonCallback &&callback) {
            _positiveTitle.swap(title);
            _positiveCallback.swap(callback);
            return *this;
        }

        Builder &setNegativeButton(std::string &&title, ButtonCallback &&callback) {
            _negativeTitle.swap(title);
            _negativeCallback.swap(callback);
            return *this;
        }

        Builder &setCancelable(bool isCancelable) { _isCancelable = isCancelable; return *this; }
        Builder &setCloseOnTouchOutside(bool isCloseOnTouchOutside) { _isCloseOnTouchOutside = isCloseOnTouchOutside; return *this; }

        AlertDialog *create();
    };

    static AlertDialog *createWithBuilder(Builder &&builder);
    bool initWithBuilder(Builder &&builder);

    void setCloseOnTouchOutside(bool isCloseOnTouchOutside) {
        _isCloseOnTouchOutside = isCloseOnTouchOutside;
    }

    void setCancelable(bool isCancelable) {
        _isCancelable = isCancelable;
    }

    void show() {
        _scene->addChild(this, 100);
        _scene = nullptr;
    }

    void dismiss() {
        this->removeFromParent();
    }

private:
    void onPositiveButton(cocos2d::Ref *sender);
    void onNegativeButton(cocos2d::Ref *sender);

    cocos2d::RefPtr<cocos2d::Scene> _scene;
    ButtonCallback _positiveCallback;
    ButtonCallback _negativeCallback;

    bool _isCloseOnTouchOutside = true;
    bool _isCancelable = true;
};

#endif
