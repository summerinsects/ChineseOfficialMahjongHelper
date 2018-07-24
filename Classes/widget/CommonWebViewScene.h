#ifndef __COMMON_WEBVIEW_SCENE_H__
#define __COMMON_WEBVIEW_SCENE_H__

#include "../BaseScene.h"

class CommonWebViewScene : public BaseScene {
public:
    enum class ContentType {
        HTML,
        URL,
        FILE
    };

    bool initWithTitle(const char *title, const std::string &content, ContentType type);
    CREATE_FUNC_WITH_PARAM_3(CommonWebViewScene, initWithTitle, const char *, title, const std::string &, content, ContentType, type);
};

#endif
