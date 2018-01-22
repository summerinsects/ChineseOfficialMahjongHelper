#include "CWCommon.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)

namespace cw {

std::string getClipboardText() {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    if ([[pasteboard types] containsObject:NSPasteboardTypeString]) {
        return [[pasteboard stringForType:NSPasteboardTypeString] UTF8String];
    }
    return "";
}

void setClipboardText(const char *text) {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard clearContents];
    [pasteboard setString:[NSString stringWithUTF8String:text] forType:NSPasteboardTypeString];
}

}

#endif

