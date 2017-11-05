#ifndef __UTILS_COMMON_H__
#define __UTILS_COMMON_H__

#include "compiler.h"
#include <string.h>
#include <string>
#include <algorithm>

namespace Common {

static FORCE_INLINE bool isCStringEmpty(const char *str) {
    return *str == '\0';
}

#ifdef _MSC_VER
std::string format(_Printf_format_string_ const char *fmt, ...);
#else
std::string format(const char *fmt, ...) FORMAT_CHECK_PRINTF(1, 2);
#endif

#ifdef _MSC_VER
void __log(_Printf_format_string_ const char *fmt, ...);
#else
void __log(const char *fmt, ...) FORMAT_CHECK_PRINTF(1, 2);
#endif

static FORCE_INLINE int __isdigit(int c) {
    return (c >= -1 && c <= 255) ? isdigit(c) : 0;
}

static FORCE_INLINE int __isspace(int c) {
    return (c >= -1 && c <= 255) ? isspace(c) : 0;
}

static inline std::string &ltrim(std::string &str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](char c) { return !__isspace(c); }));
    return str;
}

static inline std::string &rtrim(std::string &str) {
    str.erase(std::find_if(str.rbegin(), str.rend(), [](char c) { return !__isspace(c); }).base(), str.end());
    return str;
}

static inline std::string &trim(std::string &str) {
    return rtrim(ltrim(str));
}

std::string getStringFromFile(const char *file);

}

#if defined(COCOS2D_DEBUG) && (COCOS2D_DEBUG > 0)
#define MYLOG(fmt, ...)      Common::__log(fmt, ##__VA_ARGS__)
#else
#define MYLOG(fmt, ...)      (void)0
#endif

#endif
