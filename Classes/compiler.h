#ifndef _COMPILER_H_
#define _COMPILER_H_

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

// cdecl
#ifndef CDECL
#if defined(_MSC_VER)
#define CDECL __cdecl
#elif defined(__GNUC__) && defined(__i386) && !defined(__INTEL_COMPILER)
#define CDECL __attribute__((__cdecl__))
#else
#define CDECL
#endif
#endif

// stdcall
#ifndef STDCALL
#if defined(_MSC_VER)
#define STDCALL __stdcall
#elif defined(__GNUC__) && defined(__i386) && !defined(__INTEL_COMPILER)
#define STDCALL __attribute__((__stdcall__))
#else
#define STDCALL
#endif
#endif

// fastcall
#ifndef FASTCALL
#if defined(_MSC_VER) && (_MSC_VER >= 800)
#define FASTCALL __fastcall
#elif defined(__GNUC__) && defined(__i386__) && !defined(__MINGW32__)
#define FASTCALL __attribute__((__regparm__(3)))
#else
#define FASTCALL
#endif
#endif

// force inline
#ifndef FORCE_INLINE
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#define FORCE_INLINE __forceinline
#elif defined(__GNUC__) && ((__GNUC__ << 8 | __GNUC_MINOR__) >= 0x301)
#define FORCE_INLINE __attribute__((__always_inline__))
#else
#define FORCE_INLINE inline
#endif
#endif

// no inline
#ifndef NOINLINE
#if defined(_MSC_VER) && (_MSC_VER >= 1300)
#define NOINLINE __declspec(noinline)
#elif defined(__GNUC__) && ((__GNUC__ << 8 | __GNUC_MINOR__) >= 0x301)
#define NOINLINE __attribute__((noinline))
#else
#define NOINLINE
#endif
#endif

// no return
#ifndef NORETURN
#ifdef _MSC_VER
#define NORETURN __declspec(noreturn)
#elif defined(__GNUC__)
#define NORETURN __attribute__((__noreturn__))
#else
#define NORETURN
#endif
#endif

// deprecated
#ifndef DEPRECATED
#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 3))
#define DEPRECATED __attribute__((__deprecated__))
#elif defined(_MSC_VER)  && (_MSC_VER >= 1300)
#define DEPRECATED __declspec(deprecated)
#else
#define DEPRECATED
#endif
#endif

// no vtable
#ifndef NOVTABLE
#if defined(_MSC_VER) && (_MSC_VER >= 1300)
#define NOVTABLE __declspec(novtable)
#else
#define NOVTABLE
#endif
#endif

// no throw
#ifndef NOTHROW
#if defined(_MSC_VER)
#define NOTHROW __declspec(nothrow)
#elif defined(__GNUC__)
#define NOTHROW __attribute__((__nothrow__))
#else
#define NOTHROW
#endif
#endif

// returns nonnull
#ifndef RETURNS_NONULL
#if defined(_MSC_VER)
#define RETURNS_NONULL _Ret_notnull_
#elif defined(__GNUC__)
#define RETURNS_NONULL __attribute__((__returns_nonnull__))
#else
#define NOTHROW
#endif
#endif

// restrict
#ifndef RESTRICT
#if defined(_MSC_VER) && (_MSC_VER >= 1400)//1100 1300
#define RESTRICT __restrict
#elif defined(__GNUC__) && ((__GNUC__ << 8 | __GNUC_MINOR__) >= 0x291)
#define RESTRICT __restrict__
#elif defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 810)
#define RESTRICT restrict
#else
#define RESTRICT
#endif
#endif

// function signature
#ifndef FUNCSIG
#if defined(_MSC_VER) && (_MSC_VER >= 1300)
#define FUNCSIG __FUNCSIG__
#elif (defined(__GNUC__) && ((__GNUC__ << 8 | __GNUC_MINOR__) >= 0x200)) || (defined(__MWERKS__) && (__MWERKS__ > 0x3000))
#define FUNCSIG __PRETTY_FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
#define FUNCSIG __FUNC__
#else
#define FUNCSIG __FUNCTION__
#endif
#endif

// builtin expect
#ifndef BUILTIN_EXPECT
#if defined(__GNUC__) && ((__GNUC__ << 8 | __GNUC_MINOR__) >= 0x296)
#define BUILTIN_EXPECT(expr_, val_) __builtin_expect((expr_), (val_))
#else
#define BUILTIN_EXPECT(expr_, val_) (expr_)
#endif
#endif

#ifndef LIKELY
#define LIKELY(expr_) BUILTIN_EXPECT(!!(expr_), 1)
#endif

#ifndef UNLIKELY
#define UNLIKELY(expr_) BUILTIN_EXPECT(!!(expr_), 0)
#endif

#ifndef FORMAT_CHECK_PRINTF
#if defined(__GNUC__) && (__GNUC__ >= 4)
#define FORMAT_CHECK_PRINTF(format_pos_, arg_pos_) __attribute__((__format__(printf, format_pos_, arg_pos_)))
#elif defined(__has_attribute)
#if __has_attribute(format)
#define FORMAT_CHECK_PRINTF(format_pos_, arg_pos_) __attribute__((__format__(printf, format_pos_, arg_pos_)))
#endif  // __has_attribute(format)
#else
#define FORMAT_CHECK_PRINTF(format_pos_, arg_pos_)
#endif
#endif

// format for size_t
#if (defined _MSC_VER) && (_MSC_VER < 1900)
#define PRIS "Iu"
#else
#define PRIS "zu"
#endif

#ifdef ANDROID

#ifdef __cplusplus

#include <string>
#include <stdint.h>

typedef int64_t _Longlong;
typedef uint64_t _ULonglong;

#define _CSTD           ::
#define _MAX_INT_DIG    32

namespace std {

    // to_string NARROW CONVERSIONS

//#define _LLFMT "%I64"
#define _LLFMT "%ll"

#define _TOSTRING(buf, fmt, val) snprintf(buf, sizeof(buf), fmt, val)

    inline string to_string(int _Val) {  // convert int to string
        char _Buf[2 * _MAX_INT_DIG];

        _CSTD _TOSTRING(_Buf, "%d", _Val);
        return (string(_Buf));
    }

    inline string to_string(unsigned int _Val) {  // convert unsigned int to string
        char _Buf[2 * _MAX_INT_DIG];

        _CSTD _TOSTRING(_Buf, "%u", _Val);
        return (string(_Buf));
    }

    inline string to_string(long _Val) {  // convert long to string
        char _Buf[2 * _MAX_INT_DIG];

        _CSTD _TOSTRING(_Buf, "%ld", _Val);
        return (string(_Buf));
    }

    inline string to_string(unsigned long _Val) {  // convert unsigned long to string
        char _Buf[2 * _MAX_INT_DIG];

        _CSTD _TOSTRING(_Buf, "%lu", _Val);
        return (string(_Buf));
    }

    inline string to_string(_Longlong _Val) {  // convert long long to string
        char _Buf[2 * _MAX_INT_DIG];

        _CSTD _TOSTRING(_Buf, _LLFMT "d", _Val);
        return (string(_Buf));
    }

    inline string to_string(_ULonglong _Val) {  // convert unsigned long long to string
        char _Buf[2 * _MAX_INT_DIG];

        _CSTD _TOSTRING(_Buf, _LLFMT "u", _Val);
        return (string(_Buf));
    }

    inline string to_string(long double _Val) {  // convert long double to string
        typedef back_insert_iterator<string> _Iter;
        typedef num_put<char, _Iter> _Nput;
        const _Nput& _Nput_fac = use_facet<_Nput>(locale());
        ostream _Ios((streambuf *)0);
        string _Str;

        _Ios.setf(ios_base::fixed);
        _Nput_fac.put(_Iter(_Str), _Ios, ' ', _Val);
        return (_Str);
    }

    inline string to_string(double _Val) {  // convert double to string
        return (to_string((long double)_Val));
    }

    inline string to_string(float _Val) {  // convert float to string
        return (to_string((long double)_Val));
    }
}

#endif  // __cplusplus

#endif  // ANDROID

#endif
