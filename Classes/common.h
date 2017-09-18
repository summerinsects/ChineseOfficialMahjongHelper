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
template <size_t BufferSize>
static std::string format(_Printf_format_string_ const char *fmt, ...);
#else
template <size_t BufferSize>
static std::string format(const char *fmt, ...) FORMAT_CHECK_PRINTF(1, 2);
#endif

template <size_t BufferSize>
inline std::string format(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char buf[BufferSize];
    vsnprintf(buf, BufferSize, fmt, ap);
    va_end(ap);

    return buf;
}

}

#endif
