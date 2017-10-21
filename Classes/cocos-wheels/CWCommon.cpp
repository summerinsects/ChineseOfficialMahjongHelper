#include "CWCommon.h"

namespace cw {

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

}
