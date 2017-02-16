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

    namespace __basic_json_helper {

        // _FixString
        static inline const char *_FixString(char *const str) { return str; }
        static inline const char *_FixString(const char *const str) { return str; }
        template <size_t _Size> const char *_FixString(char (&str)[_Size]) { return str; }
        template <size_t _Size> const char *_FixString(const char (&str)[_Size]) { return str; }
        template <class _CharTraits, class _Allocator>
        static inline const char *_FixString(const std::basic_string<char, _CharTraits, _Allocator> &str) {
            return str.c_str();
        }

        // _IsCString
        template <class _Tp> struct _IsCStringImpl : std::false_type { };
        template <size_t _Size> struct _IsCStringImpl<char [_Size]> : std::true_type { };
        template <> struct _IsCStringImpl<char *> : std::true_type { };
        template <> struct _IsCStringImpl<const char *> : std::true_type { };

        template <class _Tp> struct _IsCString
            : _IsCStringImpl<typename std::remove_cv<typename std::remove_reference<_Tp>::type>::type> { };

        // _IsSTLString
        template <class _Tp> struct _IsSTLStringImpl : std::false_type{};
        template <class _CharTraits, class _Allocator>
        struct _IsSTLStringImpl<std::basic_string<char, _CharTraits, _Allocator> > : std::true_type { };

        template <class _Tp> struct _IsSTLString
            : _IsSTLStringImpl<typename std::remove_cv<typename std::remove_reference<_Tp>::type>::type> { };

        // _IsCArray
        template <class _Tp> struct _IsCArrayImpl : std::false_type { };
        template <class _Elem, size_t _Size>
        struct _IsCArrayImpl<_Elem [_Size]> : std::true_type { };

        template <class _Tp> struct _IsCArray
            : _IsCArrayImpl<typename std::remove_cv<typename std::remove_reference<_Tp>::type>::type> { };

        // _IsSequential
        template <class _Tp> struct _IsSequentialImpl : std::false_type { };
        template <class _Tp, class _Alloc>
        struct _IsSequentialImpl<std::vector<_Tp, _Alloc> > : std::true_type { };
        template <class _Tp, class _Alloc>
        struct _IsSequentialImpl<std::list<_Tp, _Alloc> > : std::true_type { };
        template <class _Tp, class _Alloc>
        struct _IsSequentialImpl<std::forward_list<_Tp, _Alloc> > : std::true_type { };
        template <class _Tp, class _Alloc>
        struct _IsSequentialImpl<std::deque<_Tp, _Alloc> > : std::true_type { };
        template <class _Tp, size_t _Size>
        struct _IsSequentialImpl<std::array<_Tp, _Size> > : std::true_type { };
        template <class _Tp>
        struct _IsSequentialImpl<std::initializer_list<_Tp> > : std::true_type { };

        template <class _Tp> struct _IsSequential
            : _IsSequentialImpl<typename std::remove_cv<typename std::remove_reference<_Tp>::type>::type> { };

        // _IsSet
        template <class _Tp> struct _IsSetImpl : std::false_type { };
        template <class _Tp, class _Compare, class _Alloc>
        struct _IsSetImpl<std::set<_Tp, _Compare, _Alloc> > : std::true_type { };
        template <class _Tp, class _Compare, class _Alloc>
        struct _IsSetImpl<std::multiset<_Tp, _Compare, _Alloc> > : std::true_type { };
        template <class _Tp, class _Hash, class _Pred, class _Alloc>
        struct _IsSetImpl<std::unordered_set<_Tp, _Hash, _Pred, _Alloc> > : std::true_type { };
        template <class _Tp, class _Hash, class _Pred, class _Alloc>
        struct _IsSetImpl<std::unordered_multiset<_Tp, _Hash, _Pred, _Alloc> > : std::true_type { };

        template <class _Tp> struct _IsSet
            : _IsSetImpl<typename std::remove_cv<typename std::remove_reference<_Tp>::type>::type> { };

        // _IsMap
        template <class _Tp> struct _IsMapImpl : std::false_type { };
        template <class _Key, class _Value, class _Compare, class _Alloc>
        struct _IsMapImpl<std::map<_Key, _Value, _Compare, _Alloc> > : std::true_type { };
        template <class _Key, class _Value, class _Compare, class _Alloc>
        struct _IsMapImpl<std::multimap<_Key, _Value, _Compare, _Alloc> > : std::true_type { };
        template <class _Key, class _Value, class _Hash, class _Pred, class _Alloc>
        struct _IsMapImpl<std::unordered_map<_Key, _Value, _Hash, _Pred, _Alloc> > : std::true_type { };
        template <class _Key, class _Value, class _Hash, class _Pred, class _Alloc>
        struct _IsMapImpl<std::unordered_multimap<_Key, _Value, _Hash, _Pred, _Alloc> > : std::true_type { };

        template <class _Tp> struct _IsMap
            : _IsMapImpl<typename std::remove_cv<typename std::remove_reference<_Tp>::type>::type> { };
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
        ValueType _type;  // The type of the item, as above.
        _Integer _valueint;  // The item's number, if type==Integer
        _Float _valuefloat;  // The item's number, if type==Float
        StringType _valuestring;  // The item's string, if type==String

        StringType _keystring;  // The item's name string, if this item is the child of, or is in the list of subitems of an object.
        pointer _child;  // An array or object item will have a child pointer pointing to a chain of the items in the array/object.
        pointer _next;  // next/prev allow you to walk array/object chains.
        pointer _prev;

        // 原本cJSON的实现是用的双向非循环键表，
        // 这里为了实现迭代器，增加一个头结点，用_child指向它，将头结点的_valueint用来表示链表结点数，
        // 改成了循环键表

    private:
        inline void reset() {
            _type = ValueType::Null;
            _valueint = _Integer();
            _valuefloat = _Float();
            _valuestring.clear();

            _keystring.clear();
            _child = nullptr;
            _next = nullptr;
            _prev = nullptr;
        }

    public:
        // 默认构造
        BasicJSON() { reset(); }
        ~BasicJSON() { clear(); }

        ValueType GetValueType() const { return _type; }
        const StringType &key() const { return _keystring; }

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
                    cJSON_Delete(p);
                    p = q;
                }
                cJSON_Delete(_child);
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
            _type = valueType;
            if (_type == ValueType::Array || _type == ValueType::Object) {
                _child = cJSON_New_Item();
                if (_child == nullptr) throw std::bad_alloc();
                _child->_next = _child->_prev = _child;
            }
        }

        template <class _Tp>
        explicit BasicJSON(_Tp &&val) {
            reset();
            Assign<typename std::remove_cv<typename std::remove_reference<_Tp>::type>::type>(std::forward<_Tp>(val));
        }

        template <class _Tp>
        explicit BasicJSON(const std::initializer_list<_Tp> &il) {
            reset();
            Assign<std::initializer_list<_Tp> >(il);
        }

        template <class _Tp>
        explicit BasicJSON(std::initializer_list<_Tp> &&il) {
            reset();
            Assign<std::initializer_list<_Tp> >(il);
        }

        // 复制构造
        BasicJSON(const BasicJSON &other) {
            reset();
            cJSON_Duplicate(*this, other, true);
        }

        // 移动构造
        BasicJSON(BasicJSON &&other) {
            _type = other._type;
            _valueint = other._valueint;
            _valuefloat = other._valuefloat;
            _valuestring = std::move(other._valuestring);

            _keystring = std::move(other._keystring);
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
            cJSON_Duplicate(*this, other, true);
            return *this;
        }

        // 移动赋值
        BasicJSON &operator=(BasicJSON &&other) {
            clear();

            _type = other._type;
            _valueint = other._valueint;
            _valuefloat = other._valuefloat;
            _valuestring = std::move(other._valuestring);

            _keystring = std::move(other._keystring);
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
        inline bool operator==(std::nullptr_t) const { return (_type == ValueType::Null); }
        inline bool operator!=(std::nullptr_t) const { return (_type != ValueType::Null); }

        // as
        template <class _Tp> _Tp as() const {
            return As<_Tp>();
        }

        template <class _Tp> _Tp as() {
            return As<_Tp>();
        }

        bool empty() const {
            if (_type != ValueType::Array && _type != ValueType::Object) {
                throw std::logic_error("Only Array and Object support function empty!");
            }
            return (_child->_next == _child);
        }

        typename std::make_unsigned<IntegerType>::type size() const {
            if (_type != ValueType::Array && _type != ValueType::Object) {
                throw std::logic_error("Only Array and Object support function size!");
            }
            return static_cast<typename std::make_unsigned<IntegerType>::type>(_child->_valueint);
        }

    private:

        // 从数组类容器迭代器赋值
        template <class _Iterator>
        inline void AssignFromArrayIterator(_Iterator first, _Iterator last) {
            this->_type = ValueType::Array;
            this->_child = cJSON_New_Item();
            BasicJSON *prev = this->_child;
            prev->_next = prev->_prev = prev;
            for (; first != last; ++first) {
                BasicJSON *item = cJSON_New_Item();
                item->Assign<typename std::iterator_traits<_Iterator>::value_type>(*first);
                prev->_next = item;
                item->_prev = prev;
                item->_next = this->_child;
                this->_child->_prev = item;
                ++this->_child->_valueint;
                prev = item;
            }
        }

        // 从键值对类容器迭代器赋值
        template <class _Iterator>
        void AssignFromMapIterator(_Iterator first, _Iterator last) {
            this->_type = ValueType::Object;
            this->_child = cJSON_New_Item();
            BasicJSON *prev = this->_child;
            prev->_next = prev->_prev = prev;
            for (; first != last; ++first) {
                BasicJSON *item = cJSON_New_Item();
                item->_keystring = __basic_json_helper::_FixString((*first).first);
                item->Assign<typename std::iterator_traits<_Iterator>::value_type::second_type>((*first).second);
                prev->_next = item;
                item->_prev = prev;
                item->_next = this->_child;
                this->_child->_prev = item;
                ++this->_child->_valueint;
                prev = item;
            }
        }

        // 自己
        template <class _Tp>
        void Assign(const typename std::enable_if<std::is_same<BasicJSON, _Tp>::value, _Tp>::type &arg) {
            cJSON_Duplicate(*this, arg, true);
        }

        template <class _Tp>
        void Assign(typename std::enable_if<std::is_same<BasicJSON, _Tp>::value, _Tp>::type &&arg) {
            //TODO swap
            *this = std::move(arg);
        }

        // nullptr
        template <class _Tp>
        void Assign(typename std::enable_if<std::is_same<std::nullptr_t, _Tp>::value, _Tp>::type arg) {
            _type = ValueType::Null;
        }

        // bool
        template <class _Tp>
        void Assign(typename std::enable_if<std::is_same<_Tp, bool>::value, _Tp>::type arg) {
            _type = arg ? ValueType::True : ValueType::False;
        }

        // 整数
        template <class _Tp>
        void Assign(typename std::enable_if<std::is_integral<_Tp>::value
            && !std::is_same<_Tp, bool>::value, _Tp>::type arg) {
            _type = ValueType::Integer;
            _valueint = static_cast<IntegerType>(arg);
        }

        // 枚举
        template <class _Tp>
        void Assign(typename std::enable_if<std::is_enum<_Tp>::value, _Tp>::type arg) {
            _type = ValueType::Integer;
            _valueint = static_cast<IntegerType>(arg);
        }

        // 浮点数
        template <class _Tp>
        void Assign(typename std::enable_if<std::is_floating_point<_Tp>::value, _Tp>::type arg) {
            _type = ValueType::Float;
            _valuefloat = static_cast<FloatType>(arg);
        }

        // 字符串
        template <class _Tp>
        void Assign(const typename std::enable_if<__basic_json_helper::_IsCString<_Tp>::value, _Tp>::type &arg) {
            _type = ValueType::String;
            _valuestring = __basic_json_helper::_FixString(arg);
        }

        template <class _Tp>
        void Assign(const typename std::enable_if<__basic_json_helper::_IsSTLString<_Tp>::value, _Tp>::type &arg) {
            _type = ValueType::String;
            _valuestring = __basic_json_helper::_FixString(arg);
        }

        template <class _Tp>
        void Assign(typename std::enable_if<std::is_same<StringType, _Tp>::value, _Tp>::type &&arg) {
            _type = ValueType::String;
            _valuestring = std::move(arg);
        }

        // 数组
        template <class _Tp>
        void Assign(const typename std::enable_if<__basic_json_helper::_IsCArray<_Tp>::value
            && !__basic_json_helper::_IsCString<_Tp>::value, _Tp>::type &arg) {
            AssignFromArrayIterator(std::begin(arg), std::end(arg));
        }

        template <class _Tp>
        void Assign(typename std::enable_if<__basic_json_helper::_IsCArray<_Tp>::value
            && !__basic_json_helper::_IsCString<_Tp>::value, _Tp>::type &&arg) {
            AssignFromArrayIterator(std::make_move_iterator(std::begin(arg)), std::make_move_iterator(std::end(arg)));
        }

        template <class _Tp>
        void Assign(const typename std::enable_if<__basic_json_helper::_IsSequential<_Tp>::value
                || __basic_json_helper::_IsSet<_Tp>::value, _Tp>::type &arg) {
            AssignFromArrayIterator(std::begin(arg), std::end(arg));
        }

        template <class _Tp>
        void Assign(typename std::enable_if<__basic_json_helper::_IsSequential<_Tp>::value, _Tp>::type &&arg) {
            AssignFromArrayIterator(std::make_move_iterator(std::begin(arg)), std::make_move_iterator(std::end(arg)));
        }

        // 键值对
        template <class _Tp>
        void Assign(const typename std::enable_if<__basic_json_helper::_IsMap<_Tp>::value, _Tp>::type &arg) {
            AssignFromMapIterator(arg.begin(), arg.end());
        }

        template <class _Tp>
        void Assign(typename std::enable_if<__basic_json_helper::_IsMap<_Tp>::value, _Tp>::type &&arg) {
            AssignFromMapIterator(std::make_move_iterator(arg.begin()), std::make_move_iterator(arg.end()));
        }



        // AS成自己的指针
        template <class _Tp>
        typename std::enable_if<std::is_same<const JsonType *, _Tp>::value, _Tp>::type As() const {
            return this;
        };

        template <class _Tp>
        typename std::enable_if<std::is_same<JsonType *, _Tp>::value, _Tp>::type As() {
            return this;
        };

        // AS成自己的引用
        template <class _Tp>
        typename std::enable_if<std::is_same<const JsonType &, _Tp>::value, _Tp>::type As() const {
            return *this;
        }

        template <class _Tp>
        typename std::enable_if<std::is_same<JsonType &, _Tp>::value, _Tp>::type As() {
            return *this;
        }

        // As成bool
        template <class _Tp>
        typename std::enable_if<std::is_same<bool, _Tp>::value, _Tp>::type As() const {
            switch (_type) {
            case ValueType::Null: return false;
            case ValueType::False: return false;
            case ValueType::True: return true;
            case ValueType::Integer: return !!_valueint;
            case ValueType::Float: return !!_valuefloat;
            case ValueType::String: {
                // 不支持tRUE trUE之类的奇葩输入
                if (strcmp(_valuestring.c_str(), "true") == 0 || strcmp(_valuestring.c_str(), "True") == 0
                    || strcmp(_valuestring.c_str(), "TRUE") == 0 || strcmp(_valuestring.c_str(), "1")) {
                    return true;
                }
                else if (strcmp(_valuestring.c_str(), "false") == 0 || strcmp(_valuestring.c_str(), "False") == 0
                         || strcmp(_valuestring.c_str(), "FALSE") == 0 || strcmp(_valuestring.c_str(), "0")) {
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
        template <class _Tp>
        typename std::enable_if<std::is_integral<_Tp>::value
            && !std::is_same<_Tp, bool>::value, _Tp>::type As() const {
            switch (_type) {
            case ValueType::Null: return _Tp(0);
            case ValueType::False: return _Tp(0);
            case ValueType::True: return _Tp(1);
            case ValueType::Integer: return static_cast<_Tp>(_valueint);
            case ValueType::Float: return static_cast<_Tp>(_valuefloat);
            case ValueType::String: return static_cast<_Tp>(atoll(_valuestring.c_str()));
            case ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Integer"); break;
            case ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Integer"); break;
            default: throw std::out_of_range("JSON type out of range"); break;
            }
        }

        // AS成整数或枚举
        template <class _Tp>
        typename std::enable_if<std::is_enum<_Tp>::value, _Tp>::type As() const {
            switch (_type) {
            case ValueType::Null: return _Tp(0);
            case ValueType::False: return _Tp(0);
            case ValueType::True: return _Tp(1);
            case ValueType::Integer: return static_cast<_Tp>(_valueint);
            case ValueType::Float: return static_cast<_Tp>((IntegerType)_valuefloat);
            case ValueType::String: return static_cast<_Tp>(atoll(_valuestring.c_str()));
            case ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Integer"); break;
            case ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Integer"); break;
            default: throw std::out_of_range("JSON type out of range"); break;
            }
        }

        // AS成浮点数
        template <class _Tp>
        typename std::enable_if<std::is_floating_point<_Tp>::value, _Tp>::type As() const {
            switch (_type) {
            case ValueType::Null: return _Tp(0);
            case ValueType::False: return _Tp(0);
            case ValueType::True: return _Tp(1);
            case ValueType::Integer: return static_cast<_Tp>(_valueint);
            case ValueType::Float: return static_cast<_Tp>(_valuefloat);
            case ValueType::String: return static_cast<_Tp>(atof(_valuestring.c_str()));
            case ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Float"); break;
            case ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Float"); break;
            default: throw std::out_of_range("JSON type out of range"); break;
            }
        }

        // AS成STL字符串
        template <class _Tp>
        typename std::enable_if<__basic_json_helper::_IsSTLString<_Tp>::value, _Tp>::type As() const {
            switch (_type) {
            case ValueType::Null: return _Tp();
            case ValueType::False: return _Tp("false");
            case ValueType::True: return _Tp("true");
            case ValueType::Integer: {
                char str[21];  // 2^64+1 can be represented in 21 chars.
                snprintf(str, 21, "%" PRId64, (int64_t)_valueint);
                return _Tp(str);
            }
            case ValueType::Float: {
                std::basic_ostringstream<typename _Tp::value_type, typename _Tp::traits_type, typename _Tp::allocator_type> ss;
                ss << _valuefloat;
                return ss.str();
            }
            case ValueType::String: return _Tp(_valuestring.begin(), _valuestring.end());
            case ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to String"); break;
            case ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to String"); break;
            default: throw std::out_of_range("JSON type out of range"); break;
            }
        }

        template <class _Tp> struct AsImpl {
            static inline _Tp invoke(const JsonType &ref) {
                return ref.As<_Tp>();
            }
        };

        // AS成数组类容器
        template <class _Tp>
        typename std::enable_if<__basic_json_helper::_IsSequential<_Tp>::value
            || __basic_json_helper::_IsSet<_Tp>::value, _Tp>::type As() const {
            switch (_type) {
            case ValueType::Null: return _Tp();
            case ValueType::False: throw std::logic_error("Cannot convert JSON_False to Array"); break;
            case ValueType::True: throw std::logic_error("Cannot convert JSON_True to Array"); break;
            case ValueType::Integer: throw std::logic_error("Cannot convert JSON_Integer to Array"); break;
            case ValueType::Float: throw std::logic_error("Cannot convert JSON_Float to Array"); break;
            case ValueType::String: throw std::logic_error("Cannot convert JSON_String to Array"); break;
            case ValueType::Array: {
                _Tp ret = _Tp();
                std::transform(this->begin(), this->end(), std::inserter(ret, ret.begin()),
                    &AsImpl<typename _Tp::value_type>::invoke);
                return ret;
            }
            case ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Array"); break;
            default: throw std::out_of_range("JSON type out of range"); break;
            }
        }

        template <class _Map>
        static inline typename _Map::value_type _make_value(const JsonType &json) {
            // 这里用构造第一个参数如果用typename _Map::key_type(json._keystring.begin(), json._keystring.end())会有问题
            return typename _Map::value_type(typename _Map::key_type(json._keystring.c_str()),
                json.As<typename _Map::mapped_type>());
        }

        // AS成键值对类容器
        template <class _Tp>
        typename std::enable_if<__basic_json_helper::_IsMap<_Tp>::value, _Tp>::type As() const {
            static_assert(std::is_convertible<const char *, typename _Tp::key_type>::value, "key_type must be able to convert to const char *");
            switch (_type) {
            case ValueType::Null: return _Tp();
            case ValueType::False: throw std::logic_error("Cannot convert JSON_False to Object"); break;
            case ValueType::True: throw std::logic_error("Cannot convert JSON_True to Object"); break;
            case ValueType::Integer: throw std::logic_error("Cannot convert JSON_Integer to Object"); break;
            case ValueType::Float: throw std::logic_error("Cannot convert JSON_Float to Object"); break;
            case ValueType::String: throw std::logic_error("Cannot convert JSON_String to Object"); break;
            case ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Object"); break;
            case ValueType::Object: {
                _Tp ret = _Tp();
                std::transform(this->begin(), this->end(), std::inserter(ret, ret.begin()), _make_value<_Tp>);
                return ret;
            }
            default: throw std::out_of_range("JSON type out of range"); break;
            }
        }

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
            if (_type != ValueType::Array) {
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
            if (_type != ValueType::Array) {
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
            if (_type != ValueType::Array) {
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

        template <class _Tp> iterator insert(const_iterator where, const std::initializer_list<_Tp> &il) {
            if (_type != ValueType::Array) {
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

        template <class _Tp> void insert(const std::initializer_list<_Tp> &il) {
            for (typename std::initializer_list<_Tp>::iterator it = il.begin(); it != il.end(); ++it) {
                _DoInsertForMap(*it);
            }
        }

        template <class _Tp>
        inline void push_back(_Tp &&val) {
            if (_type != ValueType::Array) {
                throw std::logic_error("Only Array support push_back!");
            }
            _DoInsertForArray(_child, std::forward<_Tp>(val));
        }

        template <class _Tp>
        inline void push_front(_Tp &&val) {
            if (_type != ValueType::Array) {
                throw std::logic_error("Only Array support push_front!");
            }
            _DoInsertForArray(_child->_next, std::forward<_Tp>(val));
        }

        iterator erase(const_iterator where) {
            if (_type != ValueType::Array && _type != ValueType::Object) {
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
            if (_type != ValueType::Array) {
                throw std::logic_error("Only Array support pop_back!");
            }
            _DoErase(_child->_prev);
        }

        inline void pop_front() {
            if (_type != ValueType::Array) {
                throw std::logic_error("Only Array support pop_front!");
            }
            _DoErase(_child->_next);
        }

        template <class _String> inline IntegerType erase(const _String &key) {
            if (_type != ValueType::Object) {
                throw std::logic_error("Only Object support erase by key!");
            }
            pointer ptr = _DoFind(__basic_json_helper::_FixString(key));
            if (ptr != nullptr) {
                _DoErase(ptr);
                return 1;
            }
            return 0;
        }

        iterator erase(const_iterator first, const_iterator last) {
            if (_type != ValueType::Array && _type != ValueType::Object) {
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
            pointer ptr = _DoFind(__basic_json_helper::_FixString(key));
            return ptr != nullptr ? iterator(ptr) : end();
        }

        template <class _String> inline const_iterator find(const _String &key) const {
            typedef typename std::conditional<std::is_array<_String>::value,
                const typename std::remove_extent<_String>::type *,
                typename std::remove_cv<_String>::type>::type FixedCStringType;
            static_assert(std::is_convertible<const char *, FixedCStringType>::value, "key_type must be able to convert to const char *");
            pointer ptr = _DoFind(__basic_json_helper::_FixString(key));
            return ptr != nullptr ? const_iterator(ptr) : end();
        }

        template <class _Tp, class _String> inline _Tp GetValueByKey(const _String &key) const {
            typedef typename std::conditional<std::is_array<_String>::value,
                const typename std::remove_extent<_String>::type *,
                typename std::remove_cv<_String>::type>::type FixedCStringType;
            static_assert(std::is_convertible<const char *, FixedCStringType>::value, "key_type must be able to convert to const char *");
            const char *str = __basic_json_helper::_FixString(key);
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
            pointer item = cJSON_New_Item();
            item->Assign<typename std::remove_cv<typename std::remove_reference<_Tp>::type>::type>(std::forward<_Tp>(val));
            if (_child->_next != _child && _child->_next->_type != item->_type) {
                cJSON_Delete(item);
                throw std::logic_error("Cannot insert a difference type into an Array.");
            }
            if (item->_prev != nullptr || item->_next != nullptr) {
                cJSON_Delete(item);
                throw std::logic_error("Item already added. It can't be added again");
            }
            ptr->_prev->_next = item;  // 连接ptr的前驱和item
            item->_prev = ptr->_prev;
            item->_next = ptr;  // 连接item和ptr的后继
            ptr->_prev = item;
            ++_child->_valueint;
            return iterator(item);
        }

        template <class _Tp> std::pair<iterator, bool> _DoInsertForMap(_Tp &&val) {
            typedef typename std::remove_cv<typename std::remove_reference<_Tp>::type>::type _PairType;
            static_assert(std::is_convertible<const char *, typename _PairType::first_type>::value, "key_type must be able to convert to const char *");
            pointer item = cJSON_New_Item();
            item->Assign<typename _PairType::second_type>(val.second);
            if (item->_prev != nullptr || item->_next != nullptr || !item->_keystring.empty()) {
                cJSON_Delete(item);
                throw std::logic_error("Item already added. It can't be added again");
            }
            const char *key = __basic_json_helper::_FixString(val.first);
            if (_DoFind(key) != nullptr) {
                char err[256];
                snprintf(err, 255, "Key: [%s] is already used.", key);
                throw std::logic_error(err);
            }
            item->_keystring = key;
            pointer ptr = _child;  // 直接插入到末尾
            ptr->_prev->_next = item;  // 连接ptr的前驱和item
            item->_prev = ptr->_prev;
            item->_next = ptr;  // 连接item和ptr的后继
            ptr->_prev = item;
            ++_child->_valueint;
            return std::make_pair(iterator(item), true);
        }

        iterator _DoErase(pointer ptr) {
            iterator ret(ptr->_next);
            ptr->_prev->_next = ptr->_next;  // 将ptr从链表中解除
            ptr->_next->_prev = ptr->_prev;
            ptr->_prev = nullptr;
            ptr->_next = nullptr;  // 设置为nullptr防止将后继结点都释放了
            cJSON_Delete(ptr);
            --_child->_valueint;
            return ret;
        }

        pointer _DoFind(const char *key) const {
            if (_type != ValueType::Object) {
                throw std::logic_error("Only Object support find by key!");
            }
            if (key == nullptr || *key == '\0') return nullptr;
            for (const_iterator it = begin(); it != end(); ++it) {
                if (strcmp(it->_keystring.c_str(), key) == 0) {  // 这里用it->_keystring.compare有问题，原因未知
                    return it._ptr;
                }
            }
            return nullptr;
        }

        static inline pointer cJSON_New_Item() {
            typedef typename _Allocator::template rebind<JsonType>::other AllocatorType;
            AllocatorType allocator;
            typename AllocatorType::pointer p = allocator.allocate(sizeof(JsonType));
            allocator.construct(p);
            return (pointer)p;
        }

        static inline void cJSON_Delete(pointer p) {
            typedef typename _Allocator::template rebind<JsonType>::other AllocatorType;
            AllocatorType allocator;
            allocator.destroy(p);
            allocator.deallocate(p, sizeof(JsonType));
        }

        static const char *skip(const char *in) {
            while (in != nullptr && *in != 0 && (unsigned char)*in <= 32) ++in;
            return in;
        }

        const char *parse_value(const char *value) {
            if (value == nullptr) return 0; // Fail on null.
            if (!strncmp(value, "null", 4)) { _type = ValueType::Null;  return value + 4; }
            if (!strncmp(value, "false", 5)) { _type = ValueType::False; return value + 5; }
            if (!strncmp(value, "true", 4)) { _type = ValueType::True; _valueint = 1; return value + 4; }
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

            _valuestring.resize(len + 1);    // This is how long we need for the string, roughly.
            typename StringType::iterator ptr2 = _valuestring.begin();

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
            _type = ValueType::String;
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
                _valuefloat = static_cast<_Float>(n);
                _type = ValueType::Float;
            } else {
                ll *= sign;
				if (std::numeric_limits<_Integer>::min() <= ll && ll <= std::numeric_limits<_Integer>::max()) {
					_valueint = static_cast<_Integer>(ll); _type = ValueType::Integer;
				} else {
				    _valuefloat = static_cast<_Float>(ll);
				    _type = ValueType::Float;
				}
            }
            return num;
        }

        const char *parse_array(const char *value) {
            pointer child;
            if (*value != '[')  { ep = value; return nullptr; } // not an array!

            value = skip(value + 1);
            if (*value == ']') return value + 1;    // empty array.

            _type = ValueType::Array;
            this->_child = cJSON_New_Item();
            if (this->_child == nullptr) return nullptr;        // memory fail
            this->_child->_next = child = cJSON_New_Item();
            if (child == nullptr) return nullptr;        // memory fail
            value = skip(child->parse_value(skip(value)));  // skip any spacing, get the value.
            if (value == nullptr) return nullptr;
            child->_next = this->_child;
            child->_prev = this->_child;
            ++this->_child->_valueint;

            while (*value == ',') {
                pointer new_item = cJSON_New_Item();
                if (new_item == nullptr) return nullptr;     // memory fail
                child->_next = new_item; new_item->_prev = child; child = new_item;
                value = skip(child->parse_value(skip(value + 1)));
                if (value == nullptr) return nullptr; // memory fail
                new_item->_next = this->_child;
                this->_child->_prev = new_item;
                ++this->_child->_valueint;
            }

            if (*value == ']') return value + 1;    // end of array
            ep = value; return nullptr; // malformed.
        }

        // Build an object from the text.
        const char *parse_object(const char *value) {
            if (*value != '{')  { ep = value; return nullptr; } // not an object!

            value = skip(value + 1);
            if (*value == '}') return value + 1;    // empty array.

            _type = ValueType::Object;
            pointer child;
            this->_child = cJSON_New_Item();
            if (this->_child == nullptr) return nullptr;        // memory fail
            this->_child->_next = child = cJSON_New_Item();
            if (child == nullptr) return nullptr;        // memory fail
            value = skip(child->parse_string(skip(value)));
            child->_type = ValueType::Null;
            if (value == nullptr) return nullptr;
            child->_keystring = std::move(child->_valuestring); child->_valuestring.clear();
            if (*value != ':') { ep = value; return nullptr; }  // fail!
            value = skip(child->parse_value(skip(value + 1)));  // skip any spacing, get the value.
            if (value == nullptr) return nullptr;
            child->_next = this->_child;
            child->_prev = this->_child;
            ++this->_child->_valueint;

            while (*value == ',') {
                pointer new_item = cJSON_New_Item();
                if (new_item == nullptr)   return nullptr; // memory fail
                child->_next = new_item; new_item->_prev = child; child = new_item;
                value = skip(child->parse_string(skip(value + 1)));
                child->_type = ValueType::Null;
                if (value == nullptr) return nullptr;
                child->_keystring = std::move(child->_valuestring); child->_valuestring.clear();
                if (*value != ':') { ep = value; return nullptr; }  // fail!
                value = skip(child->parse_value(skip(value + 1)));  // skip any spacing, get the value.
                if (value == nullptr) return nullptr;
                new_item->_next = this->_child;
                this->_child->_prev = new_item;
                ++this->_child->_valueint;
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
            switch (_type) {
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
            snprintf(str, 21, "%" PRId64, (int64_t)_valueint);
            _AppendString(ret, str);
        }

        template <class _CharSequence> void print_float(_CharSequence &ret) const {
            char str[64];  // This is a nice tradeoff.
            double d = static_cast<double>(_valuefloat);
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
            print_string_ptr(ret, _valuestring);
        }

        template <class _CharSequence> void print_array(_CharSequence &ret, int depth, bool fmt) const {
            size_t numentries = static_cast<size_t>(_child->_valueint);

            // Explicitly handle empty object case
            if (_child->_valueint == 0) {
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
            size_t numentries = static_cast<size_t>(_child->_valueint);

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
                print_string_ptr(ret, child->_keystring);
                ret.push_back(':'); if (fmt) ret.push_back('\t');
                child->print_value(ret, depth, fmt);
                if (i != numentries - 1) ret.push_back(',');
                if (fmt) ret.push_back('\n');
                child = child->_next;
            }
            if (fmt) _AppendString(ret, depth - 1, '\t');
            ret.push_back('}');
        }

        static bool cJSON_Duplicate(reference newitem, const_reference item, bool recurse) {
            newitem.clear();
            const_pointer cptr;
            pointer nptr = nullptr, newchild;
            // Copy over all vars
            newitem._type = item._type, newitem._valueint = item._valueint, newitem._valuefloat = item._valuefloat;
            newitem._valuestring = item._valuestring;
            newitem._keystring = item._keystring;
            // If non-recursive, then we're done!
            if (!recurse) return true;
            // Walk the ->next chain for the child.
            if (item._child != nullptr) {
                newitem._child = cJSON_New_Item();
                nptr = newitem._child;
                cptr = item._child->_next;
                while (cptr != item._child) {
                    newchild = cJSON_New_Item();
                    if (newchild == nullptr) return false;
                    if (!cJSON_Duplicate(*newchild, *cptr, true)) { cJSON_Delete(newchild); return false; }     // cJSON_Duplicate (with recurse) each item in the ->next chain
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
    static inline _OS &operator<<(_OS &os, const BasicJSON<_Integer, _Float, _CharTraits, _Allocator> &json) {
        os << json.Print();
        return os;
    }

    // 重载与nullptr的比较
    template <class _CharTraits, class _Integer, class _Float, class _Allocator>
    static inline bool operator==(std::nullptr_t, const BasicJSON<_Integer, _Float, _CharTraits, _Allocator> &json) throw() {
        return json.operator==(nullptr);
    }

    template <class _CharTraits, class _Integer, class _Float, class _Allocator>
    static inline bool operator!=(std::nullptr_t, const BasicJSON<_Integer, _Float, _CharTraits, _Allocator> &json) {
        return json.operator!=(nullptr);
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
