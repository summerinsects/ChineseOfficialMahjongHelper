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
    // 保存原始尺寸
    const cocos2d::Size originSize = label->getContentSize();
    if (originSize.width <= width) {
        return;
    }

    // 保存原始文本
    const std::string originText = label->getString();

    // 设置成三个点，并获取尺寸
    label->setString("...");
    const cocos2d::Size dotSize = label->getContentSize();

    // 切割比例
    const float cutWidth = originSize.width + dotSize.width - width;
    const float cutRate = cutWidth / originSize.width;

    // UTF8字符串
    const cocos2d::StringUtils::StringUTF8 utf8(originText);
    const cocos2d::StringUtils::StringUTF8::CharUTF8Store &utf8String = utf8.getString();

    // 计算按比例切割后，每部分的UTF8字符数
    const size_t totalLength = utf8String.size();
    const size_t cutLength = static_cast<size_t>(ceilf(totalLength * cutRate));
    size_t leftLength = (totalLength - cutLength) / 2;

    if (leftLength == 0) {
        label->setString(originText);
        return;
    }

    // 初始切割
    std::string newString = utf8.getAsCharSequence(0, leftLength);
    size_t partLength = newString.length();

    std::string temp = utf8.getAsCharSequence(totalLength - leftLength);
    newString.append("...").append(temp);

    // 微调
    do {
        label->setString(newString);
        if (label->getContentSize().width <= width) {
            return;
        }

        // 前半部分最后一个UTF8字符长度、后半部分第一个UTF8字符长度
        size_t charLength1 = utf8String[leftLength - 1]._char.size();
        size_t charLength2 = utf8String[totalLength - leftLength - 1]._char.size();

        // 删除前半部分的最后一个UTF8字符
        partLength -= charLength1;
        newString.erase(partLength, charLength1);

        // 删除后半部分的第一个UTF8字符，三个点长度为3
        newString.erase(partLength + 3, charLength2);
    } while (--leftLength > 0);

    label->setString(originText);
}

void calculateColumnsCenterX(const float *colWidth, size_t col, float *xPos) {
    xPos[0] = colWidth[0] * 0.5f;
    for (size_t i = 1; i < col; ++i) {
        xPos[i] = xPos[i - 1] + (colWidth[i - 1] + colWidth[i]) * 0.5f;
    }
}

}
