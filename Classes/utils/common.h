#ifndef __COMMON_H__
#define __COMMON_H__

#include "cocos2d.h"
#include "compiler.h"

#define CREATE_FUNC_WITH_PARAM_1(class_name_, init_func_, arg_type1_, arg1_)    \
static class_name_ *create(arg_type1_ arg1_) {              \
    class_name_ *ret = new (std::nothrow) class_name_();    \
    if (ret != nullptr && ret->init_func_(arg1_)) {         \
        ret->autorelease();                                 \
        return ret;                                         \
    }                                                       \
    else {                                                  \
        CC_SAFE_DELETE(ret);                                \
        return nullptr;                                     \
    }                                                       \
}

#define CREATE_FUNC_WITH_PARAM_2(class_name_, init_func_, arg_type1_, arg1_, arg_type2_, arg2_) \
static class_name_ *create(arg_type1_ arg1_, arg_type2_ arg2_) {  \
    class_name_ *ret = new (std::nothrow) class_name_();            \
    if (ret != nullptr && ret->init_func_(arg1_, arg2_)) {          \
        ret->autorelease();                                         \
        return ret;                                                 \
    }                                                               \
    else {                                                          \
        CC_SAFE_DELETE(ret);                                        \
        return nullptr;                                             \
    }                                                               \
}

#define CREATE_FUNC_WITH_PARAM_3(class_name_, init_func_, arg_type1_, arg1_, arg_type2_, arg2_, arg_type3_, arg3_)  \
static class_name_ *create(arg_type1_ arg1_, arg_type2_ arg2_, arg_type3_ arg3_) {  \
    class_name_ *ret = new (std::nothrow) class_name_();                            \
    if (ret != nullptr && ret->init_func_(arg1_, arg2_, arg3_)) {                   \
        ret->autorelease();                                                         \
        return ret;                                                                 \
    }                                                                               \
    else {                                                                          \
        CC_SAFE_DELETE(ret);                                                        \
        return nullptr;                                                             \
    }                                                                               \
}

#define CREATE_FUNC_WITH_PARAM_4(class_name_, init_func_, arg_type1_, arg1_, arg_type2_, arg2_, arg_type3_, arg3_, arg_type4_, arg4_)   \
static class_name_ *create(arg_type1_ arg1_, arg_type2_ arg2_, arg_type3_ arg3_, arg_type4_ arg4_) {    \
    class_name_ *ret = new (std::nothrow) class_name_();                                                \
    if (ret != nullptr && ret->init_func_(arg1_, arg2_, arg3_, arg4_)) {                                \
        ret->autorelease();                                                                             \
        return ret;                                                                                     \
    }                                                                                                   \
    else {                                                                                              \
        CC_SAFE_DELETE(ret);                                                                            \
        return nullptr;                                                                                 \
    }                                                                                                   \
}

namespace Common {

static inline void scaleLabelToFitWidth(cocos2d::Label *label, float width) {
    const cocos2d::Size &size = label->getContentSize();
    if (size.width > width) {
        float s = width / size.width;
        label->setScale(s);
    }
    else {
        label->setScale(1.0f);
    }
}

static inline bool isCStringEmpty(const char *str) {
    return *str == '\0';
}

#ifdef _MSC_VER
static std::string format(_Printf_format_string_ const char *fmt, ...);
#else
static std::string format(const char *fmt, ...) FORMAT_CHECK_PRINTF(1, 2);
#endif

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

#endif
