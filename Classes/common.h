#ifndef __COMMON_H__
#define __COMMON_H__

#include "cocos2d.h"
#include "compiler.h"
#include <string>

namespace Common {

static FORCE_INLINE void scaleLabelToFitWidth(cocos2d::Label *label, float width) {
    const cocos2d::Size &size = label->getContentSize();
    if (size.width > width) {
        float s = width / size.width;
        label->setScale(s);
    }
    else {
        label->setScale(1.0f);
    }
}

static FORCE_INLINE bool isCStringEmpty(const char *str) {
    return *str == '\0';
}

}

#endif
