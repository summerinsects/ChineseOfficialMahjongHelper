#include "CWCommon.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)

#include "jni/JniHelper.h"

namespace cw {

std::string getClipboardText() {
    return cocos2d::JniHelper::callStaticMethod<std::string>("org/cocos2dx/cpp/AppActivity", "getClipboardText");
}

void setClipboardText(const char *text) {
    cocos2d::JniHelper::callStaticMethod<void>("org/cocos2dx/cpp/AppActivity", "setClipboardText", text);
}

}

#endif
