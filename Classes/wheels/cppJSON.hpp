/*
  Copyright (c) 2009 Dave Gamble

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef __CPP_JSON__H__
#define __CPP_JSON__H__

#include <stddef.h>  // for ptrdiff_t
#include <stdint.h>  // for int64_t
#include <string>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>  // for PRId64
#include <string.h>  // for strncmp
#include <stdio.h>

#include <stdlib.h>  // for atoll
#include <stdexcept>
#include <type_traits>

#include <sstream>
#include <vector>
#include <list>
#include <forward_list>
#include <deque>
#include <array>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

#include <iterator>
#include <algorithm>

#ifdef _MSC_VER
#   include <crtdbg.h>
#   if _MSC_VER < 1900
#       ifdef snprintf
#           define SNPRINTF_IS_DEFINED 1
#           pragma push_macro("snprintf")
#           undef snprintf
#       else  // snprintf
#           define SNPRINTF_IS_DEFINED 0
#       endif  // snprintf
#       define snprintf sprintf_s
#   endif  // _MSC_VER < 1900
#   pragma push_macro("assert")
#   undef assert
#   define assert _ASSERTE
#else  // _MSC_VER
#   include <assert.h>
#endif  // _MSC_VER

namespace jw {

    template <class _Integer, class _Float, class _CharTraits, class _Allocator>
    class BasicJSON;

    namespace __cpp_basic_json_impl {

        // _FixString
        static inline const char *_FixString(char *const str) { return str; }
        static inline const char *_FixString(const char *const str) { return str; }
        template <size_t _Size> const char *_FixString(char (&str)[_Size]) { return str; }
        template <size_t _Size> const char *_FixString(const char (&str)[_Size]) { return str; }
        template <class _CharTraits, class _Allocator>
        static inline const char *_FixString(const std::basic_string<char, _CharTraits, _Allocator> &str) {
            return str.c_str();
        }
    }

    template <class _Integer, class _Float, class _CharTraits, class _Allocator>
    class BasicJSON {
    public:
        friend class iterator;

        enum class ValueType {
            Null, False, True, Integer, Float, String, Array, Object
        };

        typedef BasicJSON JsonType;
        typedef JsonType value_type;
        typedef value_type *pointer;
        typedef value_type &reference;
        typedef const value_type *const_pointer;
        typedef const value_type &const_reference;
        typedef _Integer size_type;
        typedef ptrdiff_t difference_type;

        typedef _Integer IntegerType;
        typedef _Float FloatType;

        // 开始作死 →_→
        typedef std::basic_string<char, _CharTraits, typename _Allocator::template rebind<char>::other> StringType;

    private:
        ValueType _valueType;  // The type of the item, as above.
        _Integer _valueInt;  // The item's number, if type==Integer
        _Float _valueFloat;  // The item's number, if type==Float
        StringType _valueString;  // The item's string, if type==String

        StringType _key;  // The item's name string, if this item is the child of, or is in the list of subitems of an object.
        pointer _child;  // An array or object item will have a child pointer pointing to a chain of the items in the array/object.
        pointer _next;  // next/prev allow you to walk array/object chains.
        pointer _prev;

        // 原本cJSON的实现是用的双向非循环键表，
        // 这里为了实现迭代器，增加一个头结点，用_child指向它，将头结点的_valueInt64用来表示链表结点数，
        // 改成了循环键表

    private:
        inline void reset() {
            _valueType = ValueType::Null;
            _valueInt = _Integer();
            _valueFloat = _Float();
            _valueString.clear();

            _key.clear();
            _child = nullptr;
            _next = nullptr;
            _prev = nullptr;
        }

    public:
        // 默认构造
        BasicJSON() { reset(); }
        ~BasicJSON() { clear(); }

        ValueType GetValueType() const { return _valueType; }
        const StringType &key() const { return _key; }

        inline bool Parse(const char *src) { return ParseWithOpts(src, nullptr, false); }

        bool ParseWithOpts(const char *src, const char **return_parse_end, bool require_null_terminated) {
            clear();
            const char *end = parse_value(skip(src));
            ep = nullptr;
            if (end == nullptr) return false;

            // if we require null-terminated JSON without appended garbage, skip and then check for a null terminator
            if (require_null_terminated) { end = skip(end); if (*end) { ep = end; return 0; } }
            if (return_parse_end) *return_parse_end = end;
            return true;
        }

        void clear() {
            if (_child != nullptr) {
                for (pointer p = _child->_next; p != _child; ) {
                    pointer q = p->_next;
                    Delete(p);
                    p = q;
                }
                Delete(_child);
                _child = nullptr;
            }
            reset();
        }

        std::string stringfiy() const {
            std::string ret;
            print_value(ret, 0, true);
            return ret;
        }

        // 带参构造
        BasicJSON(ValueType valueType) {
            reset();
            _valueType = valueType;
            if (_valueType == ValueType::Array || _valueType == ValueType::Object) {
                _child = New();
                if (_child == nullptr) throw std::bad_alloc();
                _child->_next = _child->_prev = _child;
            }
        }

        template <class _Tp>
        explicit BasicJSON(_Tp &&val) {
            reset();
            AssignImpl<typename std::remove_cv<typename std::remove_reference<_Tp>::type>::type, void>::invoke(*this, std::forward<_Tp>(val));
        }

        template <class _Tp>
        explicit BasicJSON(const std::initializer_list<_Tp> &il) {
            reset();
            AssignImpl<std::initializer_list<_Tp>, void>::invoke(*this, il);
        }

        template <class _Tp>
        explicit BasicJSON(std::initializer_list<_Tp> &&il) {
            reset();
            AssignImpl<std::initializer_list<_Tp>, void>::invoke(*this, il);
        }

        // 复制构造
        BasicJSON(const BasicJSON &other) {
            reset();
            Duplicate(*this, other, true);
        }

        // 移动构造
        BasicJSON(BasicJSON &&other) {
            _valueType = other._valueType;
            _valueInt = other._valueInt;
            _valueFloat = other._valueFloat;
            _valueString = std::move(other._valueString);

            _key = std::move(other._key);
            _child = other._child;
            _next = other._next;
            _prev = other._prev;

            other.reset();
        }

        // 用nullptr构造
        BasicJSON(std::nullptr_t) {
            reset();
        }

        // 赋值
        BasicJSON &operator=(const BasicJSON &other) {
            clear();
            Duplicate(*this, other, true);
            return *this;
        }

        // 移动赋值
        BasicJSON &operator=(BasicJSON &&other) {
            clear();

            _valueType = other._valueType;
            _valueInt = other._valueInt;
            _valueFloat = other._valueFloat;
            _valueString = std::move(other._valueString);

            _key = std::move(other._key);
            _child = other._child;
            _next = other._next;
            _prev = other._prev;

            other.reset();
            return *this;
        }

        // 用nullptr赋值
        BasicJSON &operator=(std::nullptr_t) {
            clear();
            return *this;
        }

        // 重载与nullptr的比较
        inline bool operator==(std::nullptr_t) const { return (_valueType == ValueType::Null); }
        inline bool operator!=(std::nullptr_t) const { return (_valueType != ValueType::Null); }

        // as
        template <class _Tp> _Tp as() const {
            return AsImpl<_Tp, void>::invoke(*this);
        }

        bool empty() const {
            if (_valueType != ValueType::Array && _valueType != ValueType::Object) {
                throw std::logic_error("Only Array and Object support function empty!");
            }
            return (_child->_next == _child);
        }

        typename std::make_unsigned<IntegerType>::type size() const {
            if (_valueType != ValueType::Array && _valueType != ValueType::Object) {
                throw std::logic_error("Only Array and Object support function size!");
            }
            return static_cast<typename std::make_unsigned<IntegerType>::type>(_child->_valueInt);
        }

    private:

        // 从数组类容器迭代器赋值
        template <class _Iterator>
        inline void AssignFromArrayIterator(_Iterator first, _Iterator last) {
            this->_valueType = ValueType::Array;
            this->_child = New();
            auto *prev = this->_child;
            prev->_next = prev->_prev = prev;
            for (; first != last; ++first) {
                auto *item = New();
                AssignImpl<typename std::iterator_traits<_Iterator>::value_type, void>::invoke(*item, *first);
                prev->_next = item;
                item->_prev = prev;
                item->_next = this->_child;
                this->_child->_prev = item;
                ++this->_child->_valueInt;
                prev = item;
            }
        }

        // 从键值对类容器迭代器赋值
        template <class _Iterator>
        void AssignFromMapIterator(_Iterator first, _Iterator last) {
            this->_valueType = ValueType::Object;
            this->_child = New();
            auto *prev = this->_child;
            prev->_next = prev->_prev = prev;
            for (; first != last; ++first) {
                auto *item = New();
                item->_key = __cpp_basic_json_impl::_FixString((*first).first);
                AssignImpl<typename std::iterator_traits<_Iterator>::value_type::second_type, void>::invoke(*item, (*first).second);
                prev->_next = item;
                item->_prev = prev;
                item->_next = this->_child;
                this->_child->_prev = item;
                ++this->_child->_valueInt;
                prev = item;
            }
        }

        // Assign的实现类，这里内部类貌似无法全特化，所以加了个_Dummy模板参数，使之变为偏特化
        template <class _SourceType, class _Dummy>
        struct AssignImpl;

        // 自己
        template <class _Dummy>
        struct AssignImpl<JsonType, _Dummy> {
            static inline void invoke(JsonType &ref, const JsonType &arg) {
                Duplicate(ref, arg, true);
            }
        };

        // nullptr
        template <class _Dummy>
        struct AssignImpl<std::nullptr_t, _Dummy> {
            static inline void invoke(JsonType &ref, std::nullptr_t) {
                ref.AssignFromNull();
            }
        };

        // bool
        template <class _Dummy>
        struct AssignImpl<bool, _Dummy> {
            static inline void invoke(JsonType &ref, bool arg) {
                ref._valueType = arg ? ValueType::True : ValueType::False;
            }
        };

        // 整数
        template <class _Int>
        struct AssignFromIntegerImpl {
            static inline void invoke(JsonType &ref, _Int arg) {
                ref._valueType = ValueType::Integer;
                ref._valueInt = static_cast<IntegerType>(arg);
            }
        };

        template <class _Dummy>
        struct AssignImpl<char, _Dummy> : AssignFromIntegerImpl<char> { };

        template <class _Dummy>
        struct AssignImpl<signed char, _Dummy> : AssignFromIntegerImpl<signed char> { };

        template <class _Dummy>
        struct AssignImpl<unsigned char, _Dummy> : AssignFromIntegerImpl<unsigned char> { };

        template <class _Dummy>
        struct AssignImpl<short, _Dummy> : AssignFromIntegerImpl<short> { };

        template <class _Dummy>
        struct AssignImpl<unsigned short, _Dummy> : AssignFromIntegerImpl<unsigned short> { };

        template <class _Dummy>
        struct AssignImpl<int, _Dummy> : AssignFromIntegerImpl<int> { };

        template <class _Dummy>
        struct AssignImpl<unsigned, _Dummy> : AssignFromIntegerImpl<unsigned> { };

        template <class _Dummy>
        struct AssignImpl<long, _Dummy> : AssignFromIntegerImpl<long> { };

        template <class _Dummy>
        struct AssignImpl<unsigned long, _Dummy> : AssignFromIntegerImpl<unsigned long> { };

        template <class _Dummy>
        struct AssignImpl<int64_t, _Dummy> : AssignFromIntegerImpl<int64_t> { };

        template <class _Dummy>
        struct AssignImpl<uint64_t, _Dummy> : AssignFromIntegerImpl<uint64_t> { };

        // 枚举
        template <class _Tp, class = typename std::enable_if<std::is_enum<_Tp>::value>::type>
        struct AssignFromEnumImpl {
            static inline void invoke(JsonType &ref, _Tp arg) {
                ref._valueType = ValueType::Integer;
                ref._valueInt = static_cast<IntegerType>(arg);
            }
        };

        template <class _SourceType, class _Dummy>
        struct AssignImpl : AssignFromEnumImpl<_SourceType> { };

        // 浮点数
        template <class _Flt>
        struct AssignFromFloatImpl {
            static inline void invoke(JsonType &ref, _Flt arg) {
                ref._valueType = ValueType::Float;
                ref._valueFloat = static_cast<FloatType>(arg);
            }
        };

        template <class _Dummy>
        struct AssignImpl<float, _Dummy> : AssignFromFloatImpl<float> { };

        template <class _Dummy>
        struct AssignImpl<double, _Dummy> : AssignFromFloatImpl<double> { };

        //template <class _Dummy>
        //struct AssignImpl<long double, _Dummy> : AssignFromFloatImpl<long double> { };

        // 字符串
        template <class _Str>
        struct AssignFromStringImpl {
            static inline void invoke(JsonType &ref, const _Str &arg) {
                ref._valueType = ValueType::String;
                ref._valueString = __cpp_basic_json_impl::_FixString(arg);
            }
        };

        // C风格字符串
        template <class _Dummy, size_t _Size>
        struct AssignImpl<char [_Size], _Dummy> : AssignFromStringImpl<char [_Size]> { };
        template <class _Dummy>
        struct AssignImpl<char *, _Dummy> : AssignFromStringImpl<char *> { };
        template <class _Dummy>
        struct AssignImpl<const char *, _Dummy> : AssignFromStringImpl<const char *> { };

        template <class _Dummy>
        struct AssignImpl<StringType, _Dummy> {
            static inline void invoke(JsonType &ref, const StringType &arg) {
                ref._valueType = ValueType::String;
                ref._valueString = arg;
            }
            static inline void invoke(JsonType &ref, StringType &&arg) {
                ref._valueType = ValueType::String;
                ref._valueString = std::move(arg);
            }
        };

        // STL字符串
        template <class _Dummy, class _Traits, class _Alloc>
        struct AssignImpl<std::basic_string<char, _Traits, _Alloc>, _Dummy>
            : AssignFromStringImpl<std::basic_string<char, _Traits, _Alloc> > { };

        // 传统数组
        template <class _Dummy, class _Elem, size_t _Size>
        struct AssignImpl<_Elem [_Size], _Dummy> {
            static void invoke(JsonType &ref, const _Elem (&arg)[_Size]) {
                ref.AssignFromArrayIterator(std::begin(arg), std::end(arg));
            }
            static void invoke(JsonType &ref, _Elem (&&arg)[_Size]) {
                ref.AssignFromArrayIterator(std::make_move_iterator(std::begin(arg)), std::make_move_iterator(std::end(arg)));
            }
        };

        // 数组类容器
        template <class _Array>
        struct AssignFromImmovableArrayImpl {
            static void invoke(JsonType &ref, const _Array &arg) {
                ref.AssignFromArrayIterator(arg.begin(), arg.end());
            }
        };

        template <class _Array>
        struct AssignFromMoveableArrayImpl : AssignFromImmovableArrayImpl<_Array> {
            using AssignFromImmovableArrayImpl<_Array>::invoke;
            static void invoke(JsonType &ref, _Array &&arg) {
                ref.AssignFromArrayIterator(std::make_move_iterator(arg.begin()), std::make_move_iterator(arg.end()));
            }
        };

        template <class _Dummy, class _Tp, class _Alloc>
        struct AssignImpl<std::vector<_Tp, _Alloc>, _Dummy>
            : AssignFromMoveableArrayImpl<std::vector<_Tp, _Alloc> > { };

        template <class _Dummy, class _Tp, class _Alloc>
        struct AssignImpl<std::list<_Tp, _Alloc>, _Dummy>
            : AssignFromMoveableArrayImpl<std::list<_Tp, _Alloc> > { };

        template <class _Dummy, class _Tp, class _Alloc>
        struct AssignImpl<std::forward_list<_Tp, _Alloc>, _Dummy>
            : AssignFromMoveableArrayImpl<std::forward_list<_Tp, _Alloc> > { };

        template <class _Dummy, class _Tp, class _Alloc>
        struct AssignImpl<std::deque<_Tp, _Alloc>, _Dummy>
            : AssignFromMoveableArrayImpl<std::deque<_Tp, _Alloc> > { };

        template <class _Dummy, class _Tp, size_t _Size>
        struct AssignImpl<std::array<_Tp, _Size>, _Dummy>
            : AssignFromMoveableArrayImpl<std::array<_Tp, _Size> > { };

        template <class _Dummy, class _Tp, class _Compare, class _Alloc>
        struct AssignImpl<std::set<_Tp, _Compare, _Alloc>, _Dummy>
            : AssignFromImmovableArrayImpl<std::set<_Tp, _Compare, _Alloc> > { };

        template <class _Dummy, class _Tp, class _Compare, class _Alloc>
        struct AssignImpl<std::multiset<_Tp, _Compare, _Alloc>, _Dummy>
            : AssignFromImmovableArrayImpl<std::multiset<_Tp, _Compare, _Alloc> > { };

        template <class _Dummy, class _Tp, class _Hash, class _Pred, class _Alloc>
        struct AssignImpl<std::unordered_set<_Tp, _Hash, _Pred, _Alloc>, _Dummy>
            : AssignFromImmovableArrayImpl<std::unordered_set<_Tp, _Hash, _Pred, _Alloc> > { };

        template <class _Dummy, class _Tp, class _Hash, class _Pred, class _Alloc>
        struct AssignImpl<std::unordered_multiset<_Tp, _Hash, _Pred, _Alloc>, _Dummy>
            : AssignFromImmovableArrayImpl<std::unordered_multiset<_Tp, _Hash, _Pred, _Alloc> > { };

        // 键值对类容器
        template <class _Map>
        struct AssignFromMapImpl {
            static void invoke(JsonType &ref, const _Map &arg) {
                static_assert(std::is_convertible<const char *, typename _Map::key_type>::value,
                    "key_type must be able to convert to const char *");
                ref.AssignFromMapIterator(arg.begin(), arg.end());
            }

            static void invoke(JsonType &ref, _Map &&arg) {
                static_assert(std::is_convertible<const char *, typename _Map::key_type>::value,
                    "key_type must be able to convert to const char *");
                ref.AssignFromMapIterator(std::make_move_iterator(arg.begin()), std::make_move_iterator(arg.end()));
            }
        };

        template <class _Dummy, class _Key, class _Value, class _Compare, class _Alloc>
        struct AssignImpl<std::map<_Key, _Value, _Compare, _Alloc>, _Dummy>
            : AssignFromMapImpl<std::map<_Key, _Value, _Compare, _Alloc>> { };

        template <class _Dummy, class _Key, class _Value, class _Compare, class _Alloc>
        struct AssignImpl<std::multimap<_Key, _Value, _Compare, _Alloc>, _Dummy >
            : AssignFromMapImpl<std::multimap<_Key, _Value, _Compare, _Alloc> > { };

        template <class _Dummy, class _Key, class _Value, class _Hash, class _Pred, class _Alloc>
        struct AssignImpl<std::unordered_map<_Key, _Value, _Hash, _Pred, _Alloc>, _Dummy>
            : AssignFromMapImpl<std::unordered_map<_Key, _Value, _Hash, _Pred, _Alloc> > { };

        template <class _Dummy, class _Key, class _Value, class _Hash, class _Pred, class _Alloc>
        struct AssignImpl<std::unordered_multimap<_Key, _Value, _Hash, _Pred, _Alloc>, _Dummy>
            : AssignFromMapImpl<std::unordered_multimap<_Key, _Value, _Hash, _Pred, _Alloc> > { };

        // C++11初始化列表
        template <class _Dummy, class _Tp>
        struct AssignImpl<std::initializer_list<_Tp>, _Dummy>
            : AssignFromMoveableArrayImpl<std::initializer_list<_Tp> > { };

        // AS成Bool
        bool AsBoolean() const {
            switch (_valueType) {
            case ValueType::Null: return false;
            case ValueType::False: return false;
            case ValueType::True: return true;
            case ValueType::Integer: return !!_valueInt;
            case ValueType::Float: return !!_valueFloat;
            case ValueType::String: {
                // 不支持tRUE trUE之类的奇葩输入
                if (strcmp(_valueString.c_str(), "true") == 0 || strcmp(_valueString.c_str(), "True") == 0
                    || strcmp(_valueString.c_str(), "TRUE") == 0 || strcmp(_valueString.c_str(), "1")) {
                    return true;
                }
                else if (strcmp(_valueString.c_str(), "false") == 0 || strcmp(_valueString.c_str(), "False") == 0
                         || strcmp(_valueString.c_str(), "FALSE") == 0 || strcmp(_valueString.c_str(), "0")) {
                    return false;
                }
                else {
                    throw std::logic_error("Cannot convert JSON_String to bool"); break;
                }
            }
            case ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to bool"); break;
            case ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to bool"); break;
            default: throw std::out_of_range("JSON type out of range"); break;
            }
        }

        // AS成整数
        template <class _Int>
        _Int AsInteger() const {
            switch (_valueType) {
            case ValueType::Null: return _Int(0);
            case ValueType::False: return _Int(0);
            case ValueType::True: return _Int(1);
            case ValueType::Integer: return static_cast<_Int>(_valueInt);
            case ValueType::Float: return static_cast<_Int>(_valueFloat);
            case ValueType::String: return static_cast<_Int>(atoll(_valueString.c_str()));
            case ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Integer"); break;
            case ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Integer"); break;
            default: throw std::out_of_range("JSON type out of range"); break;
            }
        }

        // AS成浮点数
        template <class _Flt>
        _Flt AsFloat() const {
            switch (_valueType) {
            case ValueType::Null: return _Flt(0);
            case ValueType::False: return _Flt(0);
            case ValueType::True: return _Flt(1);
            case ValueType::Integer: return static_cast<_Flt>(_valueInt);
            case ValueType::Float: return static_cast<_Flt>(_valueFloat);
            case ValueType::String: return static_cast<_Flt>(atof(_valueString.c_str()));
            case ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Float"); break;
            case ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Float"); break;
            default: throw std::out_of_range("JSON type out of range"); break;
            }
        }

        // AS成STL字符串
        template <class _Str>
        _Str AsString() const {
            switch (_valueType) {
            case ValueType::Null: return _Str();
            case ValueType::False: return _Str("false");
            case ValueType::True: return _Str("true");
            case ValueType::Integer: {
                char str[21];  // 2^64+1 can be represented in 21 chars.
                snprintf(str, 21, "%" PRId64, (int64_t)_valueInt);
                return _Str(str);
            }
            case ValueType::Float: {
                std::basic_ostringstream<typename _Str::value_type, typename _Str::traits_type, typename _Str::allocator_type> ss;
                ss << _valueFloat;
                return ss.str();
            }
            case ValueType::String: return _Str(_valueString.begin(), _valueString.end());
            case ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to String"); break;
            case ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to String"); break;
            default: throw std::out_of_range("JSON type out of range"); break;
            }
        }

        // AS成数组类容器
        template <class _Array>
        _Array AsArray() const {
            switch (_valueType) {
            case ValueType::Null: return _Array();
            case ValueType::False: throw std::logic_error("Cannot convert JSON_False to Array"); break;
            case ValueType::True: throw std::logic_error("Cannot convert JSON_True to Array"); break;
            case ValueType::Integer: throw std::logic_error("Cannot convert JSON_Integer to Array"); break;
            case ValueType::Float: throw std::logic_error("Cannot convert JSON_Float to Array"); break;
            case ValueType::String: throw std::logic_error("Cannot convert JSON_String to Array"); break;
            case ValueType::Array: {
                _Array ret = _Array();
                std::transform(this->begin(), this->end(), std::inserter(ret, ret.begin()),
                    &AsImpl<typename _Array::value_type, void>::invoke);
                return ret;
            }
            case ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Array"); break;
            default: throw std::out_of_range("JSON type out of range"); break;
            }
        }

        template <class _Map>
        static inline typename _Map::value_type _make_value(const JsonType &json) {
            // 这里用构造第一个参数如果用typename _Map::key_type(json._key.begin(), json._key.end())会有问题
            return typename _Map::value_type(typename _Map::key_type(json._key.c_str()),
                AsImpl<typename _Map::mapped_type, void>::invoke(json));
        }

        // AS成键值对类容器
        template <class _Map>
        _Map AsMap() const {
            static_assert(std::is_convertible<const char *, typename _Map::key_type>::value, "key_type must be able to convert to const char *");
            switch (_valueType) {
            case ValueType::Null: return _Map();
            case ValueType::False: throw std::logic_error("Cannot convert JSON_False to Object"); break;
            case ValueType::True: throw std::logic_error("Cannot convert JSON_True to Object"); break;
            case ValueType::Integer: throw std::logic_error("Cannot convert JSON_Integer to Object"); break;
            case ValueType::Float: throw std::logic_error("Cannot convert JSON_Float to Object"); break;
            case ValueType::String: throw std::logic_error("Cannot convert JSON_String to Object"); break;
            case ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Object"); break;
            case ValueType::Object: {
                _Map ret = _Map();
                std::transform(this->begin(), this->end(), std::inserter(ret, ret.begin()), _make_value<_Map>);
                return ret;
            }
            default: throw std::out_of_range("JSON type out of range"); break;
            }
        }

        // AS
        template <class _Tp, class _Dummy> struct AsImpl;

        // AS成自己的指针
        template <class _Dummy> struct AsImpl<const JsonType *, _Dummy> {
            static inline const JsonType *invoke(const JsonType &ref) {
                return &ref;
            }
        };

        // AS成自己的常引用
        template <class _Dummy> struct AsImpl<const JsonType &, _Dummy> {
            static inline const JsonType &invoke(const JsonType &ref) {
                return ref;
            }
        };

        // AS成Bool
        template <class _Dummy> struct AsImpl<bool, _Dummy> {
            static inline bool invoke(const JsonType &ref) {
                return ref.AsBool();
            }
        };

        // AS成整数
        template <class _Int> struct AsIntegerImpl {
            static inline _Int invoke(const JsonType &ref) {
                return ref.AsInteger<_Int>();
            }
        };

        template <class _Dummy> struct AsImpl<char,           _Dummy> : AsIntegerImpl<char> { };
        template <class _Dummy> struct AsImpl<signed char,    _Dummy> : AsIntegerImpl<signed char> { };
        template <class _Dummy> struct AsImpl<unsigned char,  _Dummy> : AsIntegerImpl<unsigned char> { };
        template <class _Dummy> struct AsImpl<short,          _Dummy> : AsIntegerImpl<short> { };
        template <class _Dummy> struct AsImpl<unsigned short, _Dummy> : AsIntegerImpl<unsigned short> { };
        template <class _Dummy> struct AsImpl<int,            _Dummy> : AsIntegerImpl<int> { };
        template <class _Dummy> struct AsImpl<unsigned,       _Dummy> : AsIntegerImpl<unsigned> { };
        template <class _Dummy> struct AsImpl<long,           _Dummy> : AsIntegerImpl<long> { };
        template <class _Dummy> struct AsImpl<unsigned long,  _Dummy> : AsIntegerImpl<unsigned long> { };
        template <class _Dummy> struct AsImpl<int64_t,        _Dummy> : AsIntegerImpl<int64_t> { };
        template <class _Dummy> struct AsImpl<uint64_t,       _Dummy> : AsIntegerImpl<uint64_t> { };

        // AS成枚举
        template <class _Tp, class = typename std::enable_if<std::is_enum<_Tp>::value>::type>
        struct AsEnumImpl {
            static inline _Tp invoke(const JsonType &ref) {
                return ref.AsInteger<_Tp>();
            }
        };

        template <class _Tp, class _Dummy> struct AsImpl: AsEnumImpl<_Tp> { };

        // AS成浮点数
        template <class _Flt> struct AsFloatImpl {
            static inline _Flt invoke(const JsonType &ref) {
                return ref.AsFloat<_Flt>();
            }
        };

        template <class _Dummy> struct AsImpl<float,  _Dummy> : AsFloatImpl<float> { };
        template <class _Dummy> struct AsImpl<double, _Dummy> : AsFloatImpl<double> { };

        // AS成STL字符串
        template <class _Str> struct AsStringImpl {
            static inline _Str invoke(const JsonType &ref) {
                return ref.AsString<_Str>();
            }
        };

        template <class _Dummy, class _Char, class _Traits, class _Alloc>
        struct AsImpl<std::basic_string<_Char, _Traits, _Alloc>, _Dummy>
            : AsStringImpl<std::basic_string<_Char, _Traits, _Alloc> > { };

        // AS成数组类容器
        template <class _Array> struct AsArrayImpl {
            static inline _Array invoke(const JsonType &ref) {
                return ref.AsArray<_Array>();
            }
        };

        template <class _Dummy, class _Tp, class _Alloc>
        struct AsImpl<std::vector<_Tp, _Alloc>, _Dummy>
            : AsArrayImpl<std::vector<_Tp, _Alloc> > { };

        template <class _Dummy, class _Tp, class _Alloc>
        struct AsImpl<std::list<_Tp, _Alloc>, _Dummy>
            : AsArrayImpl<std::list<_Tp, _Alloc> > { };

        template <class _Dummy, class _Tp, class _Alloc>
        struct AsImpl<std::deque<_Tp, _Alloc>, _Dummy>
            : AsArrayImpl<std::deque<_Tp, _Alloc> > { };

        template <class _Dummy, class _Tp, class _Compare, class _Alloc>
        struct AsImpl<std::set<_Tp, _Compare, _Alloc>, _Dummy>
            : AsArrayImpl<std::set<_Tp, _Compare, _Alloc> > { };

        template <class _Dummy, class _Tp, class _Compare, class _Alloc>
        struct AsImpl<std::multiset<_Tp, _Compare, _Alloc>, _Dummy>
            : AsArrayImpl<std::multiset<_Tp, _Compare, _Alloc> > { };

        template <class _Dummy, class _Tp, class _Hash, class _Pred, class _Alloc>
        struct AsImpl<std::unordered_set<_Tp, _Hash, _Pred, _Alloc>, _Dummy>
            : AsArrayImpl<std::unordered_set<_Tp, _Hash, _Pred, _Alloc> > { };

        template <class _Dummy, class _Tp, class _Hash, class _Pred, class _Alloc>
        struct AsImpl<std::unordered_multiset<_Tp, _Hash, _Pred, _Alloc>, _Dummy>
            : AsArrayImpl<std::unordered_multiset<_Tp, _Hash, _Pred, _Alloc> > { };

        // AS成键值对类容器
        template <class _Map> struct AsMapImpl {
            static inline _Map invoke(const JsonType &ref) {
                return ref.AsMap<_Map>();
            }
        };

        template <class _Dummy, class _String, class _Value, class _Compare, class _Alloc>
        struct AsImpl<std::map<_String, _Value, _Compare, _Alloc>, _Dummy>
            : AsMapImpl<std::map<_String, _Value, _Compare, _Alloc> > { };

        template <class _Dummy, class _String, class _Value, class _Compare, class _Alloc>
        struct AsImpl<std::multimap<_String, _Value, _Compare, _Alloc>, _Dummy>
            : AsMapImpl<std::multimap<_String, _Value, _Compare, _Alloc> > { };

        template <class _Dummy, class _String, class _Value, class _Hash, class _Pred, class _Alloc>
        struct AsImpl<std::unordered_map<_String, _Value, _Hash, _Pred, _Alloc>, _Dummy>
            : AsMapImpl<std::unordered_map<_String, _Value, _Hash, _Pred, _Alloc> > { };

        template <class _Dummy, class _String, class _Value, class _Hash, class _Pred, class _Alloc>
        struct AsImpl<std::unordered_multimap<_String, _Value, _Hash, _Pred, _Alloc>, _Dummy>
            : AsMapImpl<std::unordered_multimap<_String, _Value, _Hash, _Pred, _Alloc> > { };

    public:
        // 迭代器相关
        class iterator : public std::iterator<std::bidirectional_iterator_tag, value_type> {
            friend class BasicJSON;
            friend class const_iterator;

            JsonType *_ptr;

            iterator(JsonType *ptr) throw() : _ptr(ptr) { }

        public:
            iterator() throw() : _ptr(nullptr) { }
            iterator(const iterator &other) throw() : _ptr(other._ptr) { }

            iterator &operator=(const iterator &other) throw() {
                _ptr = other._ptr;
                return *this;
            }

            inline reference operator*() throw() { return *_ptr; }
            inline const_reference operator*() const throw() { return *_ptr; }
            inline pointer operator->() throw() { return _ptr; }
            inline const_pointer operator->() const throw() { return _ptr; }

            inline iterator &operator++() throw() {
                _ptr = _ptr->_next;
                return *this;
            }
            inline iterator operator++(int) throw() {
                iterator ret(this);
                _ptr = _ptr->_next;
                return ret;
            }

            inline iterator &operator--() throw() {
                _ptr = _ptr->_prev;
                return *this;
            }
            inline iterator operator--(int) throw() {
                iterator ret(this);
                _ptr = _ptr->_prev;
                return ret;
            }

            inline bool operator==(const iterator &other) const throw() { return _ptr == other._ptr; }
            inline bool operator!=(const iterator &other) const throw() { return _ptr != other._ptr; }
        };

        typedef std::reverse_iterator<iterator> reverse_iterator;

        class const_iterator : public std::iterator<std::bidirectional_iterator_tag, const value_type> {
            friend class BasicJSON;
            JsonType *_ptr;

            const_iterator(JsonType *ptr) throw() : _ptr(ptr) { }

        public:
            const_iterator() throw() : _ptr(nullptr) { }
            const_iterator(const const_iterator &other) throw() : _ptr(other._ptr) { }
            const_iterator(const typename value_type::iterator &other) throw() : _ptr(other._ptr) { }

            const_iterator &operator=(const const_iterator &other) throw() {
                _ptr = other._ptr;
                return *this;
            }

            inline reference operator*() const throw() { return *_ptr; }
            inline pointer operator->() const throw() { return _ptr; }

            inline const_iterator &operator++() throw() {
                _ptr = _ptr->_next;
                return *this;
            }
            inline const_iterator operator++(int) throw() {
                const_iterator ret(this);
                _ptr = _ptr->_next;
                return ret;
            }

            inline const_iterator &operator--() throw() {
                _ptr = _ptr->_prev;
                return *this;
            }
            inline const_iterator operator--(int) throw() {
                const_iterator ret(this);
                _ptr = _ptr->_prev;
                return ret;
            }

            inline bool operator==(const const_iterator &other) const throw() { return _ptr == other._ptr; }
            inline bool operator!=(const const_iterator &other) const throw() { return _ptr != other._ptr; }
        };

        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        inline iterator begin() { return iterator(_child->_next); }
        inline const_iterator begin() const { return const_iterator(_child->_next); }
        inline const_iterator cbegin() const { return const_iterator(_child->_next); }

        inline iterator end() { return iterator(_child); }
        inline const_iterator end() const { return const_iterator(_child); }
        inline const_iterator cend() const { return const_iterator(_child); }

        inline reverse_iterator rbegin() { return reverse_iterator(_child); }
        inline const_reverse_iterator rbegin() const { return const_reverse_iterator(_child); }
        inline const_reverse_iterator crbegin() const { return const_reverse_iterator(_child); }

        inline reverse_iterator rend() { return reverse_iterator(_child->_next); }
        inline const_reverse_iterator rend() const { return const_reverse_iterator(_child->_next); }
        inline const_reverse_iterator crend() const { return const_reverse_iterator(_child->_next); }

        template <class _Tp> iterator insert(const_iterator where, _Tp &&val) {
            if (_valueType != ValueType::Array) {
                throw std::logic_error("Only Array support insert with position specified by iterator!");
            }
            pointer ptr = where._ptr;
#if (defined _DEBUG) || (defined DEBUG)
            assert(_RangeCheck(ptr));
#endif
            return _DoInsertForArray(ptr, std::forward<_Tp>(val));
        }

        template <class _Tp> inline std::pair<iterator, bool> insert(_Tp &&val) {
            return _DoInsertForMap(val);
        }

        template <class _Tp> iterator insert(const_iterator where, size_t n, const _Tp &val) {
            if (_valueType != ValueType::Array) {
                throw std::logic_error("Only Array support with position specified by iterator!");
            }
            pointer ptr = where._ptr;
#if (defined _DEBUG) || (defined DEBUG)
            assert(_RangeCheck(ptr));
#endif
            while (n-- > 0) {
                iterator ret = _DoInsertForArray(ptr, val);
                ptr = ret._ptr->_next;
            }
            return iterator(ptr);
        }

        template <class _InputIterator> iterator insert(const_iterator where, _InputIterator first, _InputIterator last) {
            if (_valueType != ValueType::Array) {
                throw std::logic_error("Only Array support with position specified by iterator!");
            }
            pointer ptr = where._ptr;
#if (defined _DEBUG) || (defined DEBUG)
            assert(_RangeCheck(ptr));
#endif
            for (; first != last;  ++first) {
                iterator ret = _DoInsertForArray(ptr, *first);
                ptr = ret._ptr->_next;
            }
            return iterator(ptr);
        }

        template <class _InputIterator> void insert(_InputIterator first, _InputIterator last) {
            for (; first != last; ++first) {
                _DoInsertForMap(*first);
            }
        }

        template <class _Tp> iterator insert(const_iterator where, std::initializer_list<_Tp> il) {
            if (_valueType != ValueType::Array) {
                throw std::logic_error("Only Array support with position specified by iterator!");
            }
            pointer ptr = where._ptr;
#if (defined _DEBUG) || (defined DEBUG)
            assert(_RangeCheck(ptr));
#endif
            for (typename std::initializer_list<_Tp>::iterator it = il.begin(); it != il.end(); ++it) {
                iterator ret = _DoInsertForArray(ptr, *it);
                ptr = ret._ptr->_next;
            }
            return iterator(ptr);
        }

        template <class _Tp> void insert(std::initializer_list<_Tp> il) {
            for (typename std::initializer_list<_Tp>::iterator it = il.begin(); it != il.end(); ++it) {
                _DoInsertForMap(*it);
            }
        }

        template <class _Tp>
        inline void push_back(_Tp &&val) {
            if (_valueType != ValueType::Array) {
                throw std::logic_error("Only Array support push_back!");
            }
            _DoInsertForArray(_child, std::forward<_Tp>(val));
        }

        template <class _Tp>
        inline void push_front(_Tp &&val) {
            if (_valueType != ValueType::Array) {
                throw std::logic_error("Only Array support push_front!");
            }
            _DoInsertForArray(_child->_next, std::forward<_Tp>(val));
        }

        iterator erase(const_iterator where) {
            if (_valueType != ValueType::Array && _valueType != ValueType::Object) {
                throw std::logic_error("Only Array and Object support erase!");
            }
            pointer ptr = where._ptr;
#if (defined _DEBUG) || (defined DEBUG)
            assert(_RangeCheck(ptr));
#endif
            return _DoErase(ptr);
        }

        iterator inline erase(iterator where) {
            return erase(const_iterator(where._ptr));
        }

        inline void pop_back() {
            if (_valueType != ValueType::Array) {
                throw std::logic_error("Only Array support pop_back!");
            }
            _DoErase(_child->_prev);
        }

        inline void pop_front() {
            if (_valueType != ValueType::Array) {
                throw std::logic_error("Only Array support pop_front!");
            }
            _DoErase(_child->_next);
        }

        template <class _String> inline IntegerType erase(const _String &key) {
            if (_valueType != ValueType::Object) {
                throw std::logic_error("Only Object support erase by key!");
            }
            pointer ptr = _DoFind(__cpp_basic_json_impl::_FixString(key));
            if (ptr != nullptr) {
                _DoErase(ptr);
                return 1;
            }
            return 0;
        }

        iterator erase(const_iterator first, const_iterator last) {
            if (_valueType != ValueType::Array && _valueType != ValueType::Object) {
                throw std::logic_error("Only Array and Object support erase by iterators!");
            }
            pointer ptr = first._ptr;
#if (defined _DEBUG) || (defined DEBUG)
            assert(_RangeCheck(ptr));
#endif
            iterator ret(ptr);
            while (first != last) {
                ret = _DoErase(ptr);
                ptr = ret._ptr;
                first._ptr = ptr;
            }
            return ret;
        }

        template <class _String> inline iterator find(const _String &key) {
            typedef typename std::conditional<std::is_array<_String>::value,
                const typename std::remove_extent<_String>::type *,
                typename std::remove_cv<_String>::type>::type FixedCStringType;
            static_assert(std::is_convertible<const char *, FixedCStringType>::value, "key_type must be able to convert to const char *");
            pointer ptr = _DoFind(__cpp_basic_json_impl::_FixString(key));
            return ptr != nullptr ? iterator(ptr) : end();
        }

        template <class _String> inline const_iterator find(const _String &key) const {
            typedef typename std::conditional<std::is_array<_String>::value,
                const typename std::remove_extent<_String>::type *,
                typename std::remove_cv<_String>::type>::type FixedCStringType;
            static_assert(std::is_convertible<const char *, FixedCStringType>::value, "key_type must be able to convert to const char *");
            pointer ptr = _DoFind(__cpp_basic_json_impl::_FixString(key));
            return ptr != nullptr ? const_iterator(ptr) : end();
        }

        template <class _Tp, class _String> inline _Tp GetValueByKey(const _String &key) const {
            typedef typename std::conditional<std::is_array<_String>::value,
                const typename std::remove_extent<_String>::type *,
                typename std::remove_cv<_String>::type>::type FixedCStringType;
            static_assert(std::is_convertible<const char *, FixedCStringType>::value, "key_type must be able to convert to const char *");
            const char *str = __cpp_basic_json_impl::_FixString(key);
            pointer ptr = _DoFind(str);
            if (ptr == nullptr) {
                char err[256];
                snprintf(err, 255, "Cannot find value for key: [%s]", str);
                throw std::logic_error(err);
            }
            return ptr->as<_Tp>();
        }

        template <class _Tp, class _String> inline _Tp GetValueByKeyNoThrow(const _String &key) const throw() {
            try {
                return GetValueByKey<_Tp, _String>(key);
            }
            catch (...) {
                return _Tp();
            }
        }

    private:
        const char *ep = nullptr;
        //typename _Allocator::template rebind<JsonType>::other _allocator;

		bool _RangeCheck(const_pointer ptr) const {
            if (_child != nullptr) {
                if (ptr == _child) return true;
                for (const_pointer p = _child->_next; p != _child; p = p->_next) {
                    if (ptr == p) return true;
                }
            }
            return false;
        }

        template <class _Tp> iterator _DoInsertForArray(pointer ptr, _Tp &&val) {
            pointer item = New();
            AssignImpl<typename std::remove_cv<typename std::remove_reference<_Tp>::type>::type, void>::invoke(*item, std::forward<_Tp>(val));
            if (_child->_next != _child && _child->_next->_valueType != item->_valueType) {
                Delete(item);
                throw std::logic_error("Cannot insert a difference type into an Array.");
            }
            if (item->_prev != nullptr || item->_next != nullptr) {
                Delete(item);
                throw std::logic_error("Item already added. It can't be added again");
            }
            ptr->_prev->_next = item;  // 连接ptr的前驱和item
            item->_prev = ptr->_prev;
            item->_next = ptr;  // 连接item和ptr的后继
            ptr->_prev = item;
            ++_child->_valueInt;
            return iterator(item);
        }

        template <class _Tp> std::pair<iterator, bool> _DoInsertForMap(_Tp &&val) {
            typedef typename std::remove_cv<typename std::remove_reference<_Tp>::type>::type _PairType;
            static_assert(std::is_convertible<const char *, typename _PairType::first_type>::value, "key_type must be able to convert to const char *");
            pointer item = New();
            AssignImpl<typename _PairType::second_type, void>::invoke(*item, val.second);
            if (item->_prev != nullptr || item->_next != nullptr || !item->_key.empty()) {
                Delete(item);
                throw std::logic_error("Item already added. It can't be added again");
            }
            const char *key = __cpp_basic_json_impl::_FixString(val.first);
            if (_DoFind(key) != nullptr) {
                char err[256];
                snprintf(err, 255, "Key: [%s] is already used.", key);
                throw std::logic_error(err);
            }
            item->_key = key;
            pointer ptr = _child;  // 直接插入到末尾
            ptr->_prev->_next = item;  // 连接ptr的前驱和item
            item->_prev = ptr->_prev;
            item->_next = ptr;  // 连接item和ptr的后继
            ptr->_prev = item;
            ++_child->_valueInt;
            return std::make_pair(iterator(item), true);
        }

        iterator _DoErase(pointer ptr) {
            iterator ret(ptr->_next);
            ptr->_prev->_next = ptr->_next;  // 将ptr从链表中解除
            ptr->_next->_prev = ptr->_prev;
            ptr->_prev = nullptr;
            ptr->_next = nullptr;  // 设置为nullptr防止将后继结点都释放了
            Delete(ptr);
            --_child->_valueInt;
            return ret;
        }

        pointer _DoFind(const char *key) const {
            if (_valueType != ValueType::Object) {
                throw std::logic_error("Only Object support find by key!");
            }
            if (key == nullptr || *key == '\0') return nullptr;
            for (const_iterator it = begin(); it != end(); ++it) {
                if (strcmp(it->_key.c_str(), key) == 0) {  // 这里用it->_key.compare有问题，原因未知
                    return it._ptr;
                }
            }
            return nullptr;
        }

        static inline pointer New() {
            typedef typename _Allocator::template rebind<JsonType>::other AllocatorType;
            AllocatorType allocator;
            typename AllocatorType::pointer p = allocator.allocate(sizeof(JsonType));
            allocator.construct(p);
            return (pointer)p;
        }

        static inline void Delete(pointer c) {
            typedef typename _Allocator::template rebind<JsonType>::other AllocatorType;
            AllocatorType allocator;
            allocator.destroy(c);
            allocator.deallocate(c, sizeof(JsonType));
        }

        static const char *skip(const char *in) {
            while (in != nullptr && *in != 0 && (unsigned char)*in <= 32) ++in;
            return in;
        }

        const char *parse_value(const char *value) {
            if (value == nullptr) return 0; // Fail on null.
            if (!strncmp(value, "null", 4)) { _valueType = ValueType::Null;  return value + 4; }
            if (!strncmp(value, "false", 5)) { _valueType = ValueType::False; return value + 5; }
            if (!strncmp(value, "true", 4)) { _valueType = ValueType::True; _valueInt = 1; return value + 4; }
            if (*value == '\"') { return parse_string(value); }
            if (*value == '-' || (*value >= '0' && *value <= '9')) { return parse_number(value); }
            if (*value == '[') { return parse_array(value); }
            if (*value == '{') { return parse_object(value); }

            ep = value; return nullptr; // failure.
        }

        static unsigned parse_hex4(const char *str) {
            unsigned h = 0;
            if (*str >= '0' && *str <= '9') h += (*str) - '0';
            else if (*str >= 'A' && *str <= 'F') h += 10 + (*str) - 'A';
            else if (*str >= 'a' && *str <= 'f') h += 10 + (*str) - 'a';
            else return 0;
            h = h << 4; ++str;
            if (*str >= '0' && *str <= '9') h += (*str) - '0';
            else if (*str >= 'A' && *str <= 'F') h += 10 + (*str) - 'A';
            else if (*str >= 'a' && *str <= 'f') h += 10 + (*str) - 'a';
            else return 0;
            h = h << 4; ++str;
            if (*str >= '0' && *str <= '9') h += (*str) - '0';
            else if (*str >= 'A' && *str <= 'F') h += 10 + (*str) - 'A';
            else if (*str >= 'a' && *str <= 'f') h += 10 + (*str) - 'a';
            else return 0;
            h = h << 4; ++str;
            if (*str >= '0' && *str <= '9') h += (*str) - '0';
            else if (*str >= 'A' && *str <= 'F') h += 10 + (*str) - 'A';
            else if (*str >= 'a' && *str <= 'f') h += 10 + (*str) - 'a';
            else return 0;
            return h;
        }

        const char *parse_string(const char *str) {
            static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
            const char *ptr = str + 1; int len = 0; unsigned uc, uc2;
            if (*str != '\"') { ep = str; return 0; }   // not a string!

            while (*ptr != '\"' && *ptr && ++len) if (*ptr++ == '\\') ++ptr;    // Skip escaped quotes.

            _valueString.resize(len + 1);    // This is how long we need for the string, roughly.
            typename StringType::iterator ptr2 = _valueString.begin();

            ptr = str + 1;
            while (*ptr != '\"' && *ptr) {
                if (*ptr != '\\') *ptr2++ = *ptr++;
                else {
                    ++ptr;
                    switch (*ptr) {
                    case 'b': *ptr2++ = '\b';   break;
                    case 'f': *ptr2++ = '\f';   break;
                    case 'n': *ptr2++ = '\n';   break;
                    case 'r': *ptr2++ = '\r';   break;
                    case 't': *ptr2++ = '\t';   break;
                    case 'u':    // transcode utf16 to utf8.
                        uc = parse_hex4(ptr + 1); ptr += 4; // get the unicode char.

                        if ((uc >= 0xDC00 && uc <= 0xDFFF) || uc == 0)  break;  // check for invalid.

                        if (uc >= 0xD800 && uc <= 0xDBFF) { // UTF16 surrogate pairs.
                            if (ptr[1] != '\\' || ptr[2] != 'u')    break;  // missing second-half of surrogate.
                            uc2 = parse_hex4(ptr + 3); ptr += 6;
                            if (uc2 < 0xDC00 || uc2>0xDFFF)       break;  // invalid second-half of surrogate.
                            uc = 0x10000 + (((uc & 0x3FF) << 10) | (uc2 & 0x3FF));
                        }

                        len = 4; if (uc < 0x80) len = 1; else if (uc < 0x800) len = 2; else if (uc < 0x10000) len = 3; ptr2 += len;

                        switch (len) {
                        case 4: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
                        case 3: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
                        case 2: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
                        case 1: *--ptr2 = (uc | firstByteMark[len]);
                        }
                        ptr2 += len;
                        break;
                    default:  *ptr2++ = *ptr; break;
                    }
                    ++ptr;
                }
            }
            *ptr2 = 0;
            if (*ptr == '\"') ++ptr;
            _valueType = ValueType::String;
            return ptr;
        }

        const char *parse_number(const char *num) {
            double n = 0, scale = 0;
            int sign = 1, subscale = 0, signsubscale = 1;
            int64_t ll = 0;
            bool point = false;

            if (*num == '-') sign = -1, ++num;  // Has sign?
            if (*num == '0') ++num;         // is zero
            if (*num >= '1' && *num <= '9') do n = (n*10.0) + (*num - '0'), ll = (ll * 10) + (*num++ - '0'); while (*num >= '0' && *num <= '9');    // Number?
            if (*num == '.' && num[1] >= '0' && num[1] <= '9') { point = true; ++num; do n = (n * 10.0) + (*num++ - '0'), --scale; while (*num >= '0' && *num <= '9'); }    // Fractional part?
            if (*num == 'e' || *num == 'E') {   // Exponent?
                point = true; ++num; if (*num == '+') ++num;    else if (*num == '-') signsubscale = -1, ++num;     // With sign?
                while (*num >= '0' && *num <= '9') subscale = (subscale * 10) + (*num++ - '0'); // Number?
            }

            if (point || subscale > 0) {
                n = sign * n * pow(10.0, (scale + subscale * signsubscale));    // number = +/- number.fraction * 10^+/- exponent
                _valueFloat = static_cast<_Float>(n);
                _valueType = ValueType::Float;
            } else {
                ll *= sign;
				if (std::numeric_limits<_Integer>::min() <= ll && ll <= std::numeric_limits<_Integer>::max()) {
					_valueInt = static_cast<_Integer>(ll); _valueType = ValueType::Integer;
				} else {
				    _valueFloat = static_cast<_Float>(ll);
				    _valueType = ValueType::Float;
				}
            }
            return num;
        }

        const char *parse_array(const char *value) {
            pointer child;
            if (*value != '[')  { ep = value; return nullptr; } // not an array!

            value = skip(value + 1);
            if (*value == ']') return value + 1;    // empty array.

            _valueType = ValueType::Array;
            this->_child = New();
            if (this->_child == nullptr) return nullptr;        // memory fail
            this->_child->_next = child = New();
            if (child == nullptr) return nullptr;        // memory fail
            value = skip(child->parse_value(skip(value)));  // skip any spacing, get the value.
            if (value == nullptr) return nullptr;
            child->_next = this->_child;
            child->_prev = this->_child;
            ++this->_child->_valueInt;

            while (*value == ',') {
                pointer new_item = New();
                if (new_item == nullptr) return nullptr;     // memory fail
                child->_next = new_item; new_item->_prev = child; child = new_item;
                value = skip(child->parse_value(skip(value + 1)));
                if (value == nullptr) return nullptr; // memory fail
                new_item->_next = this->_child;
                this->_child->_prev = new_item;
                ++this->_child->_valueInt;
            }

            if (*value == ']') return value + 1;    // end of array
            ep = value; return nullptr; // malformed.
        }

        // Build an object from the text.
        const char *parse_object(const char *value) {
            if (*value != '{')  { ep = value; return nullptr; } // not an object!

            value = skip(value + 1);
            if (*value == '}') return value + 1;    // empty array.

            _valueType = ValueType::Object;
            pointer child;
            this->_child = New();
            if (this->_child == nullptr) return nullptr;        // memory fail
            this->_child->_next = child = New();
            if (child == nullptr) return nullptr;        // memory fail
            value = skip(child->parse_string(skip(value)));
            child->_valueType = ValueType::Null;
            if (value == nullptr) return nullptr;
            child->_key = std::move(child->_valueString); child->_valueString.clear();
            if (*value != ':') { ep = value; return nullptr; }  // fail!
            value = skip(child->parse_value(skip(value + 1)));  // skip any spacing, get the value.
            if (value == nullptr) return nullptr;
            child->_next = this->_child;
            child->_prev = this->_child;
            ++this->_child->_valueInt;

            while (*value == ',') {
                pointer new_item = New();
                if (new_item == nullptr)   return nullptr; // memory fail
                child->_next = new_item; new_item->_prev = child; child = new_item;
                value = skip(child->parse_string(skip(value + 1)));
                child->_valueType = ValueType::Null;
                if (value == nullptr) return nullptr;
                child->_key = std::move(child->_valueString); child->_valueString.clear();
                if (*value != ':') { ep = value; return nullptr; }  // fail!
                value = skip(child->parse_value(skip(value + 1)));  // skip any spacing, get the value.
                if (value == nullptr) return nullptr;
                new_item->_next = this->_child;
                this->_child->_prev = new_item;
                ++this->_child->_valueInt;
            }

            if (*value == '}') return value + 1;    // end of array
            ep = value; return nullptr; // malformed.
        }

        template <class _CharSequence>
        static inline void _AppendString(_CharSequence &ret, const char *str) {
            while (*str != '\0') {
                ret.push_back(*str++);
            }
        }

        template <class _CharSequence>
        static inline void _AppendString(_CharSequence &ret, size_t n, char ch) {
            while (n-- > 0) {
                ret.push_back(ch);
            }
        }

        template <class _CharSequence> void print_value(_CharSequence &ret, int depth, bool fmt) const {
            switch (_valueType) {
            case ValueType::Null: _AppendString(ret, "null"); break;
            case ValueType::False: _AppendString(ret, "false"); break;
            case ValueType::True: _AppendString(ret, "true"); break;
            case ValueType::Integer: print_integer(ret); break;
            case ValueType::Float: print_float(ret); break;
            case ValueType::String: print_string(ret); break;
            case ValueType::Array: print_array(ret, depth, fmt); break;
            case ValueType::Object: print_object(ret, depth, fmt); break;
            default: break;
            }
        }

        template <class _CharSequence> void print_integer(_CharSequence &ret) const {
            char str[21];  // 2^64+1 can be represented in 21 chars.
            snprintf(str, 21, "%" PRId64, (int64_t)_valueInt);
            _AppendString(ret, str);
        }

        template <class _CharSequence> void print_float(_CharSequence &ret) const {
            char str[64];  // This is a nice tradeoff.
            double d = static_cast<double>(_valueFloat);
            if (fabs(floor(d) - d) <= std::numeric_limits<double>::epsilon() && fabs(d) < 1.0e60) snprintf(str, 64, "%.0f", d);
            else if (fabs(d) < 1.0e-6 || fabs(d) > 1.0e9) snprintf(str, 64, "%e", d);
            else snprintf(str, 64, "%f", d);
            _AppendString(ret, str);
        }

        template <class _CharSequence> static void print_string_ptr(_CharSequence &ret, const StringType &str) {
            if (str.empty()) {
                ret.push_back('\"');
                ret.push_back('\"');
                return;
            }

            const char *ptr; int len = 0; unsigned char token;
            ptr = str.c_str(); while ((token = *ptr) && ++len) { if (strchr("\"\\\b\f\n\r\t", token)) ++len; else if (token < 32) len += 5; ++ptr; }

            ret.reserve(ret.size() + len + 3);
            ptr = str.c_str();
            ret.push_back('\"');
            while (*ptr) {
                if ((unsigned char)*ptr > 31 && *ptr != '\"' && *ptr != '\\') ret.push_back(*ptr++);
                else {
                    ret.push_back('\\');
                    switch (token = *ptr++) {
                    case '\\':  ret.push_back('\\'); break;
                    case '\"':  ret.push_back('\"'); break;
                    case '\b':  ret.push_back('b'); break;
                    case '\f':  ret.push_back('f'); break;
                    case '\n':  ret.push_back('n'); break;
                    case '\r':  ret.push_back('r'); break;
                    case '\t':  ret.push_back('t'); break;
                    default: { char ptr2[8]; snprintf(ptr2, 8, "u%04x", token); _AppendString(ret, ptr2); } break;  // escape and print
                    }
                }
            }
            ret.push_back('\"');
        }

        template <class _CharSequence> void print_string(_CharSequence &ret) const {
            print_string_ptr(ret, _valueString);
        }

        template <class _CharSequence> void print_array(_CharSequence &ret, int depth, bool fmt) const {
            size_t numentries = static_cast<size_t>(_child->_valueInt);

            // Explicitly handle empty object case
            if (_child->_valueInt == 0) {
                _AppendString(ret, "[]");
                return;
            }

            // Retrieve all the results:
            pointer child = _child;
            size_t i = 0;
            ret.push_back('[');
            for (child = _child->_next; child != _child; child = child->_next, ++i) {
                child->print_value(ret, depth + 1, fmt);
                if (i != numentries - 1) { ret.push_back(','); if (fmt) ret.push_back(' '); }
            }
            ret.push_back(']');
        }

        template <class _CharSequence> void print_object(_CharSequence &ret, int depth, bool fmt) const {
            size_t numentries = static_cast<size_t>(_child->_valueInt);

            // Explicitly handle empty object case
            if (numentries == 0) {
                ret.push_back('{');
                if (fmt) { ret.push_back('\n'); if (depth > 0) _AppendString(ret, depth - 1, '\t'); }
                ret.push_back('}');
                return;
            }

            // Compose the output:
            pointer child = _child->_next;
            ++depth;
            ret.push_back('{'); if (fmt) ret.push_back('\n');
            for (size_t i = 0; i < numentries; ++i) {
                if (fmt) _AppendString(ret, depth, '\t');
                print_string_ptr(ret, child->_key);
                ret.push_back(':'); if (fmt) ret.push_back('\t');
                child->print_value(ret, depth, fmt);
                if (i != numentries - 1) ret.push_back(',');
                if (fmt) ret.push_back('\n');
                child = child->_next;
            }
            if (fmt) _AppendString(ret, depth - 1, '\t');
            ret.push_back('}');
        }

        static bool Duplicate(reference newitem, const_reference item, bool recurse) {
            newitem.clear();
            const_pointer cptr;
            pointer nptr = nullptr, newchild;
            // Copy over all vars
            newitem._valueType = item._valueType, newitem._valueInt = item._valueInt, newitem._valueFloat = item._valueFloat;
            newitem._valueString = item._valueString;
            newitem._key = item._key;
            // If non-recursive, then we're done!
            if (!recurse) return true;
            // Walk the ->next chain for the child.
            if (item._child != nullptr) {
                newitem._child = New();
                nptr = newitem._child;
                cptr = item._child->_next;
                while (cptr != item._child) {
                    newchild = New();
                    if (newchild == nullptr) return false;
                    if (!Duplicate(*newchild, *cptr, true)) { Delete(newchild); return false; }     // Duplicate (with recurse) each item in the ->next chain
                    nptr->_next = newchild, newchild->_prev = nptr; nptr = newchild;    // crosswire ->prev and ->next and move on
                    cptr = cptr->_next;
                }
                newitem._child->_prev = nptr;
                nptr->_next = newitem._child;
            }
            return true;
        }

    public:
        inline std::string Print() const {
            std::string ret;
            print_value(ret, 0, true);
            return ret;
        }

        inline std::string PrintUnformatted() const {
            std::string ret;
            print_value(ret, 0, false);
            return ret;
        }

        template <class _CharContainer>
        inline void PrintTo(_CharContainer &container, bool format) const {
            print_value(container, 0, format);
        }

        static void Minify(char *json) {
            char *into = json;
            while (*json) {
                if (*json == ' ') ++json;
                else if (*json == '\t') ++json; // Whitespace characters.
                else if (*json == '\r') ++json;
                else if (*json == '\n') ++json;
                else if (*json == '/' && json[1] == '/')  while (*json && *json != '\n') ++json;    // double-slash comments, to end of line.
                else if (*json == '/' && json[1] == '*') { while (*json && !(*json == '*' && json[1] == '/')) ++json; json += 2; }  // multiline comments.
                else if (*json == '\"'){ *into++ = *json++; while (*json && *json != '\"'){ if (*json == '\\') *into++ = *json++; *into++ = *json++; }*into++ = *json++; } // string literals, which are \" sensitive.
                else *into++ = *json++;         // All other characters.
            }
            *into = 0;  // and null-terminate.
        }
    };

    // 流输出
    template <class _OS, class _Integer, class _Float, class _CharTraits, class _Allocator>
    static inline _OS &operator<<(_OS &os, const BasicJSON<_Integer, _Float, _CharTraits, _Allocator> &c) {
        os << c.Print();
        return os;
    }

    // 重载与nullptr的比较
    template <class _CharTraits, class _Integer, class _Float, class _Allocator>
    static inline bool operator==(std::nullptr_t, const BasicJSON<_Integer, _Float, _CharTraits, _Allocator> &c) throw() {
        return c.operator==(nullptr);
    }

    template <class _CharTraits, class _Integer, class _Float, class _Allocator>
    static inline bool operator!=(std::nullptr_t, const BasicJSON<_Integer, _Float, _CharTraits, _Allocator> &c) {
        return c.operator!=(nullptr);
    }

    typedef BasicJSON<int64_t, double, std::char_traits<char>, std::allocator<char> > cppJSON;
}

#ifdef _MSC_VER
#   if _MSC_VER < 1900
#       undef snprintf
#       if SNPRINTF_IS_DEFINED
#           pragma pop_macro("snprintf")
#       endif  // SNPRINTF_IS_DEFINED
#       undef SNPRINTF_IS_DEFINED
#   endif  // _MSC_VER < 1900
#   undef assert
#   pragma pop_macro("assert")
#endif  // _MSC_VER

#endif
