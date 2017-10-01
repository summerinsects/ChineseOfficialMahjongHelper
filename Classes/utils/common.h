#ifndef __UTILS_COMMON_H__
#define __UTILS_COMMON_H__

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

#define CREATE_FUNC_WITH_PARAM_2(class_name_, init_func_, arg_type1_, arg1_,    \
    arg_type2_, arg2_)                                              \
static class_name_ *create(arg_type1_ arg1_, arg_type2_ arg2_) {    \
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

#define CREATE_FUNC_WITH_PARAM_3(class_name_, init_func_, arg_type1_, arg1_,    \
    arg_type2_, arg2_, arg_type3_, arg3_)                                       \
static class_name_ *create(arg_type1_ arg1_, arg_type2_ arg2_,                  \
    arg_type3_ arg3_) {                                                         \
    class_name_ *ret = new (std::nothrow) class_name_();                        \
    if (ret != nullptr && ret->init_func_(arg1_, arg2_, arg3_)) {               \
        ret->autorelease();                                                     \
        return ret;                                                             \
    }                                                                           \
    else {                                                                      \
        CC_SAFE_DELETE(ret);                                                    \
        return nullptr;                                                         \
    }                                                                           \
}

#define CREATE_FUNC_WITH_PARAM_4(class_name_, init_func_, arg_type1_, arg1_,    \
    arg_type2_, arg2_, arg_type3_, arg3_, arg_type4_, arg4_)                    \
static class_name_ *create(arg_type1_ arg1_, arg_type2_ arg2_,                  \
    arg_type3_ arg3_, arg_type4_ arg4_) {                                       \
    class_name_ *ret = new (std::nothrow) class_name_();                        \
    if (ret != nullptr && ret->init_func_(arg1_, arg2_, arg3_, arg4_)) {        \
        ret->autorelease();                                                     \
        return ret;                                                             \
    }                                                                           \
    else {                                                                      \
        CC_SAFE_DELETE(ret);                                                    \
        return nullptr;                                                         \
    }                                                                           \
}

#define CREATE_FUNC_WITH_PARAM_5(class_name_, init_func_, arg_type1_, arg1_,    \
    arg_type2_, arg2_, arg_type3_, arg3_, arg_type4_, arg4_, arg_type5_, arg5_) \
static class_name_ *create(arg_type1_ arg1_, arg_type2_ arg2_,                  \
    arg_type3_ arg3_, arg_type4_ arg4_, arg_type5_ arg5_) {                     \
    class_name_ *ret = new (std::nothrow) class_name_();                        \
    if (ret != nullptr && ret->init_func_(arg1_, arg2_, arg3_, arg4_, arg5_)) { \
        ret->autorelease();                                                     \
        return ret;                                                             \
    }                                                                           \
    else {                                                                      \
        CC_SAFE_DELETE(ret);                                                    \
        return nullptr;                                                         \
    }                                                                           \
}

namespace Common {

void scaleLabelToFitWidth(cocos2d::Label *label, float width);

void calculateColumnsCenterX(const float *colWidth, size_t col, float *xPos);

static inline bool isCStringEmpty(const char *str) {
    return *str == '\0';
}

#ifdef _MSC_VER
std::string format(_Printf_format_string_ const char *fmt, ...);
#else
std::string format(const char *fmt, ...) FORMAT_CHECK_PRINTF(1, 2);
#endif

}

#endif
