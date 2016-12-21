#ifndef __COMMON_H__
#define __COMMON_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"

#pragma execution_character_set("utf-8")

static const char *tilesImageName[] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "tiles/m1.png", "tiles/m2.png", "tiles/m3.png", "tiles/m4.png", "tiles/m5.png", "tiles/m6.png", "tiles/m7.png", "tiles/m8.png", "tiles/m9.png", "", "", "", "", "", "",
    "", "tiles/s1.png", "tiles/s2.png", "tiles/s3.png", "tiles/s4.png", "tiles/s5.png", "tiles/s6.png", "tiles/s7.png", "tiles/s8.png", "tiles/s9.png", "", "", "", "", "", "",
    "", "tiles/p1.png", "tiles/p2.png", "tiles/p3.png", "tiles/p4.png", "tiles/p5.png", "tiles/p6.png", "tiles/p7.png", "tiles/p8.png", "tiles/p9.png", "", "", "", "", "", "",
    "", "tiles/w1.png", "tiles/w2.png", "tiles/w3.png", "tiles/w4.png", "", "", "", "", "", "", "", "", "", "", "",
    "", "tiles/d1.png", "tiles/d2.png", "tiles/d3.png", "", "", "", "", "", "", "", "", "", "", "", "",
};

static const char *handNameText[] = { "东风东", "东风南", "东风西", "东风北", "南风东", "南风南", "南风西", "南风北",
    "西风东", "西风南", "西风西", "西风北", "北风东", "北风南", "北风西", "北风北"};

static void adjustSystemFontSize(cocos2d::Label *label, float width) {
    const cocos2d::Size &size = label->getContentSize();
    if (size.width > width) {
        float s = floorf(label->getSystemFontSize() * width / size.width);
        label->setSystemFontSize(s > FLT_EPSILON ? s : 1);
    }
}

#endif
