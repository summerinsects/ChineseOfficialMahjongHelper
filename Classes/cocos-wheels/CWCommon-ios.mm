#include "CWCommon.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)

#include <UIKit/UIKit.h>

namespace cw {

std::string getClipboardText() {
    NSString *str = [UIPasteboard generalPasteboard].string;
    return [str UTF8String];
}

void setClipboardText(const char *text) {
    UIPasteboard *pasteboard = [UIPasteboard generalPasteboard];
    pasteboard.string = [NSString stringWithUTF8String:text];
}

}

#endif

