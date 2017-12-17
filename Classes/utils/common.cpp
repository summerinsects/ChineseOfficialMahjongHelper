#include "common.h"
#include <stdarg.h>

// android
#if defined(ANDROID)
#include <android/log.h>
#endif

// win32
#if defined(_WIN32) && defined(_WINDOWS)
#include <windows.h>
#endif

namespace Common {

namespace {

std::string __format(const char *fmt, va_list ap) {
    std::string ret;

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
            va_list temp;
            va_copy(temp, ap);
            int size = vsnprintf(&ret[0], len, fmt, temp);
            va_end(temp);
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

std::string format(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::string ret = __format(fmt, ap);
    va_end(ap);
    return std::move(ret);
}

void __log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::string ret = __format(fmt, ap);
    va_end(ap);
    ret.append(1, '\n');

#if defined(ANDROID)
    __android_log_print(ANDROID_LOG_DEBUG, "debug info", "%s", ret.c_str());
#elif defined(_WIN32) && defined(_WINDOWS)
    std::wstring wszBuf;
    int len = ::MultiByteToWideChar(CP_UTF8, 0, ret.c_str(), -1, nullptr, 0);
    if (len > 0) {
        wszBuf.resize(len);
        ::MultiByteToWideChar(CP_UTF8, 0, ret.c_str(), -1, &wszBuf[0], len);
        ::OutputDebugStringW(wszBuf.c_str());
    }
#else
    // Linux, Mac, iOS, etc
    fprintf(stdout, "%s", ret.c_str());
    fflush(stdout);
#endif
}

std::string getStringFromFile(const char *file) {
    std::string str;
    FILE *fp = fopen(file, "rb");
    if (LIKELY(fp != nullptr)) {
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        try {
            str.resize(size + 1);
            fread(&str[0], sizeof(char), size, fp);
        }
        catch (...) {
        }
        fclose(fp);
    }
    return str;
}

}
