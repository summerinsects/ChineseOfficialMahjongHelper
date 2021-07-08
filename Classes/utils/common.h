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

template <size_t N1, size_t N2>
inline char *strncpy(char (&dst)[N1], const char (&src)[N2]) {
    static_assert(N1 > 0 && N2 > 0, "");
    size_t len = std::min(N1, N2) - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

class StringView {
public:
    StringView() {}
    StringView(const char *str) : _data(str), _size(strlen(str)) {}
    StringView(const char *str, size_t len) : _data(str), _size(len) {}
    StringView(const std::string &str) : _data(str.data()), _size(str.size()) {}

    const char *data() const { return _data; }
    size_t size() const { return _size; }

private:
    const char *_data{};
    size_t _size{};
};

template <size_t N>
inline char *strncpy(char (&dst)[N], StringView src) {
    static_assert(N > 0, "");
    size_t len = std::min(N - 1, src.size());
    memcpy(dst, src.data(), len);
    dst[len] = '\0';
    return dst;
}

std::string getStringFromFile(const char *file);

bool compareVersion(const char *remote, const char *local);
}

#if defined(COCOS2D_DEBUG) && (COCOS2D_DEBUG > 0)
#define MYLOG(fmt, ...)      Common::__log(fmt, ##__VA_ARGS__)
#else
#define MYLOG(fmt, ...)      (void)0
#endif


#ifndef _MSC_VER
#ifndef __cplusplus
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#else
extern "C++" {
    template <typename _CountofType, size_t _SizeOfArray>
    char (*__countof_helper(/*UNALIGNED*/ _CountofType (&_Array)[_SizeOfArray]))[_SizeOfArray];
}
#define _countof(_Array) sizeof(*__countof_helper(_Array))
#endif
#endif

#endif
