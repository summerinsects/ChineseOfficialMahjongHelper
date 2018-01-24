#include "CWCommon.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)

namespace cw {

std::string getClipboardText() {
    std::string ret;
    if (::OpenClipboard(NULL)) {
        HGLOBAL hGlobal = ::GetClipboardData(CF_UNICODETEXT);
        if (hGlobal != NULL) {
            LPWSTR lpszData = (LPWSTR)::GlobalLock(hGlobal);
            if (lpszData != nullptr) {
                int size = ::WideCharToMultiByte(CP_UTF8, 0, lpszData, -1, nullptr, 0, nullptr, nullptr);
                if (size > 0) {
                    ret.resize(size + 1);
                    ::WideCharToMultiByte(CP_UTF8, 0, lpszData, -1, &ret[0], size, nullptr, nullptr);
                }
                ::GlobalUnlock(hGlobal);
            }
        }
        ::CloseClipboard();
    }
    return ret;
}

void setClipboardText(const char *text) {
    int size = ::MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (size < 0) {
        return;
    }

    if (::OpenClipboard(NULL)) {
        ::EmptyClipboard();
        HGLOBAL hGlobal = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, (size + 1) * sizeof(WCHAR));
        if (hGlobal != NULL) {
            LPWSTR lpszData = (LPWSTR)::GlobalLock(hGlobal);
            if (lpszData != nullptr) {
                ::MultiByteToWideChar(CP_UTF8, 0, text, -1, lpszData, size);
                ::GlobalUnlock(hGlobal);
                ::SetClipboardData(CF_UNICODETEXT, hGlobal);
            }
        }
        ::CloseClipboard();
    }
}

}

#endif
