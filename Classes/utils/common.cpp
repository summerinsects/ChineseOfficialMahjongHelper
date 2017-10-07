#include "common.h"

namespace Common {

void scaleLabelToFitWidth(cocos2d::Label *label, float width) {
    const cocos2d::Size &size = label->getContentSize();
    if (size.width > width) {
        float s = width / size.width;
        label->setScale(s);
    }
    else {
        label->setScale(1.0f);
    }
}

void calculateColumnsCenterX(const float *colWidth, size_t col, float *xPos) {
    xPos[0] = colWidth[0] * 0.5f;
    for (size_t i = 1; i < col; ++i) {
        xPos[i] = xPos[i - 1] + (colWidth[i - 1] + colWidth[i]) * 0.5f;
    }
}

void calculateRankFromScore(const int (&scores)[4], unsigned (&ranks)[4]) {
    memset(ranks, 0, sizeof(ranks));
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (i == j) continue;
            if (scores[i] < scores[j]) ++ranks[i];
            //if (scores[i] == scores[j] && i > j) ++ranks[i];  // 这一行的作用是取消并列
        }
    }
}

std::string format(const char *fmt, ...) {
    std::string ret;
    va_list ap;

    size_t fmtlen = strlen(fmt);
    if (LIKELY(fmtlen < INT_MAX)) {  // Ensure fmtlen is in an int
        int len = static_cast<int>(fmtlen) + 1;

        // For each %, reserve 64 characters
        for (const char *p = strchr(fmt, '%'); p != nullptr; p = strchr(p, '%')) {
            if (*++p != '%') len += 64;  // skip %%. issue: '\0'!='%' is true
        }

        do {
            ret.resize(len);

            va_start(ap, fmt);
            int size = vsnprintf(&ret[0], len, fmt, ap);
            va_end(ap);

            if (LIKELY(size >= 0)) {
                if (LIKELY(size < len)) {  // Everything worked
                    ret.resize(size);
                    ret.shrink_to_fit();
                    break;
                }
                len = size + 1;  // Needed size returned
                continue;
            }
            len *= 2;  // Guess at a larger size
        } while (1);
    }

    return ret;
}

}
