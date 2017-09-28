#ifndef __UTILS_TO_STRING_H__
#define __UTILS_TO_STRING_H__

// Android dose not have std::to_string
// The following implementation is from VS2013
#ifdef ANDROID

#ifdef __cplusplus

//
// Copyright (c) 1992-2012 by P.J. Plauger.  ALL RIGHTS RESERVED.
// Consult your license regarding permissions and restrictions.
// V6.00:0009

#include <string>
#include <stdint.h>

namespace std {

typedef int64_t _Longlong;
typedef uint64_t _ULonglong;

#define _CSTD           ::
#define _MAX_INT_DIG    32

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

    inline string to_string(int64_t _Val) {  // convert long long to string
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

#undef _CSTD
#undef _MAX_INT_DIG
#undef _LLFMT
#undef _TOSTRING

}

#endif  // __cplusplus

#endif  // ANDROID

#endif
