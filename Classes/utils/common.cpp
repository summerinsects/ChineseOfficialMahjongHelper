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

void trimLabelStringWithEllipsisToFitWidth(cocos2d::Label *label, float width) {
    const cocos2d::Size orginSize = label->getContentSize();
    if (orginSize.width <= width) {
        return;
    }

    const std::string orginText = label->getString();

    label->setString("...");
    const cocos2d::Size dotSize = label->getContentSize();

    const float cutWidth = orginSize.width + dotSize.width - width;
    const float cutRate = cutWidth / orginSize.width * 0.5f;

    const cocos2d::StringUtils::StringUTF8 utf8(orginText);
    const cocos2d::StringUtils::StringUTF8::CharUTF8Store &utf8String = utf8.getString();

    const size_t totalLength = utf8String.size();
    const size_t cutLength = static_cast<size_t>(ceilf(totalLength * cutRate));
    size_t leftLength = (totalLength - cutLength) / 2;

    if (leftLength == 0) {
        label->setString(orginText);
        return;
    }

    std::string newString = utf8.getAsCharSequence(0, leftLength);
    size_t partLength = newString.length();

    std::string temp = utf8.getAsCharSequence(totalLength - leftLength);
    newString.append("...").append(temp);

    do {
        label->setString(newString);
        if (label->getContentSize().width <= width) {
            return;
        }

        size_t l1 = utf8String[leftLength]._char.size();
        size_t l2 = utf8String[totalLength - leftLength]._char.size();
        newString.erase(partLength - l1, l1);
        partLength -= l1;
        newString.erase(partLength + 3, l2);
    } while (--leftLength > 0);

    label->setString(orginText);
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
            char ch = *++p;
            if (ch != '%' && ch != '\0') len += 64;  // skip %%
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
