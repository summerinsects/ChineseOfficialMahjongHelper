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

    template <class _Integer, class _Float, class _Traits, class _Alloc>
    class BasicJSON;

    namespace __cpp_basic_json_impl {

        // _FixString
        static inline const char *_FixString(char *const str) { return str; }
        static inline const char *_FixString(const char *const str) { return str; }
        template <size_t _Size> const char *_FixString(char (&str)[_Size]) { return str; }
        template <size_t _Size> const char *_FixString(const char (&str)[_Size]) { return str; }
        template <class _Traits, class _Alloc>
        static inline const char *_FixString(const std::basic_string<char, _Traits, _Alloc> &str) {
            return str.c_str();
        }

        // AssignImpl
        template <class _JsonType, class _SourceType> struct AssignImpl {
            typedef _SourceType SourceType;
            static void invoke(_JsonType &c, SourceType arg);
        };

        template <class _JsonType, class _Integer> struct AssignFromIntegerImpl;
        template <class _JsonType, class _Float> struct AssignFromFloatImpl;
        template <class _JsonType, class _String, bool _Moveable> struct AssignFromStringImpl;

        template <class _JsonType, class Iterator>
        void _AssignFromArrayHelper(_JsonType &c, Iterator first, Iterator last);

        template <class _JsonType, class Iterator>
        void _AssignFromMapHelper(_JsonType &c, Iterator first, Iterator last);

        // AsImpl
        template <class _JsonType, class _TargetType> struct AsImpl {
            typedef _TargetType TargetType;
            static TargetType invoke(const _JsonType &c);
        };

        template <class _JsonType, class _Integer> struct AsIntegerImpl;
        template <class _JsonType, class _Float> struct AsFloatImpl;
        template <class _JsonType, class _String> struct AsStringImpl;
        template <class _JsonType, class _Array> struct AsArrayImpl;
        template <class _JsonType, class _Map> struct AsMapImpl;
    }

    template <class _Integer, class _Float, class _Traits, class _Alloc>
    class BasicJSON {
    public:
        friend class iterator;

        enum class ValueType {
            Null, False, True, Integer, Float, String, Array, Object
        };

        typedef BasicJSON<_Integer, _Float, _Traits, _Alloc> value_type;
        typedef value_type *pointer;
        typedef value_type &reference;
        typedef const value_type *const_pointer;
        typedef const value_type &const_reference;
        typedef _Integer size_type;
        typedef ptrdiff_t difference_type;

        typedef _Integer IntegerType;
        typedef _Float FloatType;

        // 开始作死 →_→
        typedef std::basic_string<char, _Traits, typename _Alloc::template rebind<char>::other> StringType;

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
        BasicJSON<_Integer, _Float, _Traits, _Alloc>() { reset(); }
        ~BasicJSON<_Integer, _Float, _Traits, _Alloc>() { clear(); }

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
        BasicJSON<_Integer, _Float, _Traits, _Alloc>(ValueType valueType) {
            reset();
            _valueType = valueType;
            if (_valueType == ValueType::Array || _valueType == ValueType::Object) {
                _child = New();
                if (_child == nullptr) throw std::bad_alloc();
                _child->_next = _child->_prev = _child;
            }
        }

        template <class _T>
        explicit BasicJSON<_Integer, _Float, _Traits, _Alloc>(_T &&val) {
            reset();
            __cpp_basic_json_impl::AssignImpl<BasicJSON<_Integer, _Float, _Traits, _Alloc>,
                typename std::remove_cv<typename std::remove_reference<_T>::type>::type>::invoke(*this, std::forward<_T>(val));
        }

        template <class _T>
        explicit BasicJSON<_Integer, _Float, _Traits, _Alloc>(const std::initializer_list<_T> &il) {
            reset();
            __cpp_basic_json_impl::AssignImpl<BasicJSON<_Integer, _Float, _Traits, _Alloc>, std::initializer_list<_T> >::invoke(*this, il);
        }

        template <class _T>
        explicit BasicJSON<_Integer, _Float, _Traits, _Alloc>(std::initializer_list<_T> &&il) {
            reset();
            __cpp_basic_json_impl::AssignImpl<BasicJSON<_Integer, _Float, _Traits, _Alloc>, std::initializer_list<_T> >::invoke(*this, il);
        }

        // 复制构造
        BasicJSON<_Integer, _Float, _Traits, _Alloc>(const BasicJSON<_Integer, _Float, _Traits, _Alloc> &other) {
            reset();
            Duplicate(*this, other, true);
        }

        // 移动构造
        BasicJSON<_Integer, _Float, _Traits, _Alloc>(BasicJSON<_Integer, _Float, _Traits, _Alloc> &&other) {
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
        BasicJSON<_Integer, _Float, _Traits, _Alloc>(std::nullptr_t) {
            reset();
        }

        // 赋值
        BasicJSON<_Integer, _Float, _Traits, _Alloc> &operator=(const BasicJSON<_Integer, _Float, _Traits, _Alloc> &other) {
            clear();
            Duplicate(*this, other, true);
            return *this;
        }

        // 移动赋值
        BasicJSON<_Integer, _Float, _Traits, _Alloc> &operator=(BasicJSON<_Integer, _Float, _Traits, _Alloc> &&other) {
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
        BasicJSON<_Integer, _Float, _Traits, _Alloc> &operator=(std::nullptr_t) {
            clear();
            return *this;
        }

        // 重载与nullptr的比较
        inline bool operator==(std::nullptr_t) const { return (_valueType == ValueType::Null); }
        inline bool operator!=(std::nullptr_t) const { return (_valueType != ValueType::Null); }

        // as
        template <class _T> _T as() const {
            return __cpp_basic_json_impl::AsImpl<BasicJSON<_Integer, _Float, _Traits, _Alloc>, _T>::invoke(*this);
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

    public:
        // 迭代器相关
        class iterator : public std::iterator<std::bidirectional_iterator_tag, value_type> {
            friend class BasicJSON<_Integer, _Float, _Traits, _Alloc>;
            friend class const_iterator;

            BasicJSON<_Integer, _Float, _Traits, _Alloc> *_ptr;

            iterator(BasicJSON<_Integer, _Float, _Traits, _Alloc> *ptr) throw() : _ptr(ptr) { }

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
            friend class BasicJSON<_Integer, _Float, _Traits, _Alloc>;
            BasicJSON<_Integer, _Float, _Traits, _Alloc> *_ptr;

            const_iterator(BasicJSON<_Integer, _Float, _Traits, _Alloc> *ptr) throw() : _ptr(ptr) { }

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

        template <class _T> iterator insert(const_iterator where, _T &&val) {
            if (_valueType != ValueType::Array) {
                throw std::logic_error("Only Array support insert with position specified by iterator!");
            }
            pointer ptr = where._ptr;
#if (defined _DEBUG) || (defined DEBUG)
            assert(_RangeCheck(ptr));
#endif
            return _DoInsertForArray(ptr, std::forward<_T>(val));
        }

        template <class _T> inline std::pair<iterator, bool> insert(_T &&val) {
            return _DoInsertForMap(val);
        }

        template <class _T> iterator insert(const_iterator where, size_t n, const _T &val) {
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

        template <class _T> iterator insert(const_iterator where, std::initializer_list<_T> il) {
            if (_valueType != ValueType::Array) {
                throw std::logic_error("Only Array support with position specified by iterator!");
            }
            pointer ptr = where._ptr;
#if (defined _DEBUG) || (defined DEBUG)
            assert(_RangeCheck(ptr));
#endif
            for (typename std::initializer_list<_T>::iterator it = il.begin(); it != il.end(); ++it) {
                iterator ret = _DoInsertForArray(ptr, *it);
                ptr = ret._ptr->_next;
            }
            return iterator(ptr);
        }

        template <class _T> void insert(std::initializer_list<_T> il) {
            for (typename std::initializer_list<_T>::iterator it = il.begin(); it != il.end(); ++it) {
                _DoInsertForMap(*it);
            }
        }

        template <class _T>
        inline void push_back(_T &&val) {
            if (_valueType != ValueType::Array) {
                throw std::logic_error("Only Array support push_back!");
            }
            _DoInsertForArray(_child, std::forward<_T>(val));
        }

        template <class _T>
        inline void push_front(_T &&val) {
            if (_valueType != ValueType::Array) {
                throw std::logic_error("Only Array support push_front!");
            }
            _DoInsertForArray(_child->_next, std::forward<_T>(val));
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
            if (_valueType != ValueType::Object) {
                throw std::logic_error("Only Object support find by key!");
            }
            pointer ptr = _DoFind(__cpp_basic_json_impl::_FixString(key));
            return ptr != nullptr ? iterator(ptr) : end();
        }

        template <class _String> inline const_iterator find(const _String &key) const {
            typedef typename std::conditional<std::is_array<_String>::value,
                const typename std::remove_extent<_String>::type *,
                typename std::remove_cv<_String>::type>::type FixedCStringType;
            static_assert(std::is_convertible<const char *, FixedCStringType>::value, "key_type must be able to convert to const char *");
            if (_valueType != ValueType::Object) {
                throw std::logic_error("Only Object support find by key!");
            }
            pointer ptr = _DoFind(__cpp_basic_json_impl::_FixString(key));
            return ptr != nullptr ? const_iterator(ptr) : end();
        }

        template <class _T, class _String> inline _T GetValueByKey(const _String &key) const {
            typedef typename std::conditional<std::is_array<_String>::value,
                const typename std::remove_extent<_String>::type *,
                typename std::remove_cv<_String>::type>::type FixedCStringType;
            static_assert(std::is_convertible<const char *, FixedCStringType>::value, "key_type must be able to convert to const char *");
            if (_valueType != ValueType::Object) {
                throw std::logic_error("Only Object support find by key!");
            }
            const char *str = __cpp_basic_json_impl::_FixString(key);
            pointer ptr = _DoFind(str);
            if (ptr == nullptr) {
                char err[256];
                snprintf(err, 255, "Cannot find value for key: [%s]", str);
                throw std::logic_error(err);
            }
            return ptr->as<_T>();
        }

        template <class _T, class _String> inline _T GetValueByKeyNoThrow(const _String &key) const {
            try {
                return GetValueByKey<_T, _String>(key);
            }
            catch (...) {
                return _T();
            }
        }

        template <class, class> friend struct __cpp_basic_json_impl::AssignImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AssignFromIntegerImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AssignFromFloatImpl;
        template <class, class, bool> friend struct __cpp_basic_json_impl::AssignFromStringImpl;

        template <class _JsonType, class Iterator>
        friend void __cpp_basic_json_impl::_AssignFromArrayHelper(_JsonType &c, Iterator first, Iterator last);

        template <class _JsonType, class Iterator>
        friend void __cpp_basic_json_impl::_AssignFromMapHelper(_JsonType &c, Iterator first, Iterator last);

        template <class, class> friend struct __cpp_basic_json_impl::AsImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AsIntegerImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AsFloatImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AsStringImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AsArrayImpl;
        template <class, class> friend struct __cpp_basic_json_impl::AsMapImpl;

    private:
        const char *ep = nullptr;
        //typename _Alloc::template rebind<BasicJSON<_Integer, _Float, _Traits, _Alloc> >::other _allocator;

		bool _RangeCheck(const_pointer ptr) const {
            if (_child != nullptr) {
                if (ptr == _child) return true;
                for (const_pointer p = _child->_next; p != _child; p = p->_next) {
                    if (ptr == p) return true;
                }
            }
            return false;
        }

        template <class _T> iterator _DoInsertForArray(pointer ptr, _T &&val) {
            pointer item = New();
            __cpp_basic_json_impl::AssignImpl<BasicJSON<_Integer, _Float, _Traits, _Alloc>,
                typename std::remove_cv<typename std::remove_reference<_T>::type>::type>::invoke(*item, std::forward<_T>(val));
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

        template <class _T> std::pair<iterator, bool> _DoInsertForMap(_T &&val) {
            typedef typename std::remove_cv<typename std::remove_reference<_T>::type>::type _PairType;
            static_assert(std::is_convertible<const char *, typename _PairType::first_type>::value, "key_type must be able to convert to const char *");
            pointer item = New();
            __cpp_basic_json_impl::AssignImpl<BasicJSON<_Integer, _Float, _Traits, _Alloc>,
                typename _PairType::second_type>::invoke(*item, val.second);
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
            typedef typename _Alloc::template rebind<BasicJSON<_Integer, _Float, _Traits, _Alloc> >::other AllocatorType;
            AllocatorType allocator;
            typename AllocatorType::pointer p = allocator.allocate(sizeof(BasicJSON<_Integer, _Float, _Traits, _Alloc>));
            allocator.construct(p);
            return (pointer)p;
        }

        static inline void Delete(pointer c) {
            typedef typename _Alloc::template rebind<BasicJSON<_Integer, _Float, _Traits, _Alloc> >::other AllocatorType;
            AllocatorType allocator;
            allocator.destroy(c);
            allocator.deallocate(c, sizeof(BasicJSON<_Integer, _Float, _Traits, _Alloc>));
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
    template <class _OS, class _Integer, class _Float, class _Traits, class _Alloc>
    static inline _OS &operator<<(_OS &os, const BasicJSON<_Integer, _Float, _Traits, _Alloc> &c) {
        os << c.Print();
        return os;
    }

    // 重载与nullptr的比较
    template <class _Traits, class _Integer, class _Float, class _Alloc>
    static inline bool operator==(std::nullptr_t, const BasicJSON<_Integer, _Float, _Traits, _Alloc> &c) throw() {
        return c.operator==(nullptr);
    }

    template <class _Traits, class _Integer, class _Float, class _Alloc>
    static inline bool operator!=(std::nullptr_t, const BasicJSON<_Integer, _Float, _Traits, _Alloc> &c) {
        return c.operator!=(nullptr);
    }

    namespace __cpp_basic_json_impl {

        //
        // Assign
        //

        // 相同类型
        template <class _JsonType> struct AssignImpl<_JsonType, _JsonType> {
            typedef _JsonType SourceType;
            static inline void invoke(_JsonType &c, const _JsonType &arg) {
                c = arg;
            }
            static inline void invoke(_JsonType &c, _JsonType &&arg) {
                c = std::move(arg);
            }
        };

        // nullptr
        template <class _JsonType> struct AssignImpl<_JsonType, std::nullptr_t> {
            typedef std::nullptr_t SourceType;
            static inline void invoke(_JsonType &c, SourceType) {
                c._valueType = _JsonType::ValueType::Null;
            }
        };

        // bool
        template <class _JsonType> struct AssignImpl<_JsonType, bool> {
            typedef bool SourceType;
            static inline void invoke(_JsonType &c, SourceType src) {
                c._valueType = src ? _JsonType::ValueType::True : _JsonType::ValueType::False;
            }
        };

        // 整数实现
        template <class _JsonType, class _Integer> struct AssignFromIntegerImpl {
            typedef _Integer SourceType;
            static inline void invoke(_JsonType &c, SourceType arg) {
                c._valueType = _JsonType::ValueType::Integer;
                c._valueInt = static_cast<typename _JsonType::IntegerType>(arg);
            }
        };

        // 整数
        template <class _JsonType> struct AssignImpl<_JsonType, char>
            : AssignFromIntegerImpl<_JsonType, char> { };
        template <class _JsonType> struct AssignImpl<_JsonType, signed char>
            : AssignFromIntegerImpl<_JsonType, signed char> { };
        template <class _JsonType> struct AssignImpl<_JsonType, unsigned char>
            : AssignFromIntegerImpl<_JsonType, unsigned char> { };
        template <class _JsonType> struct AssignImpl<_JsonType, short>
            : AssignFromIntegerImpl<_JsonType, short> { };
        template <class _JsonType> struct AssignImpl<_JsonType, unsigned short>
            : AssignFromIntegerImpl<_JsonType, unsigned short> { };
        template <class _JsonType> struct AssignImpl<_JsonType, int>
            : AssignFromIntegerImpl<_JsonType, int> { };
        template <class _JsonType> struct AssignImpl<_JsonType, unsigned>
            : AssignFromIntegerImpl<_JsonType, unsigned> { };
        template <class _JsonType> struct AssignImpl<_JsonType, long>
            : AssignFromIntegerImpl<_JsonType, long> { };
        template <class _JsonType> struct AssignImpl<_JsonType, unsigned long>
            : AssignFromIntegerImpl<_JsonType, unsigned long> { };
        template <class _JsonType> struct AssignImpl<_JsonType, int64_t>
            : AssignFromIntegerImpl<_JsonType, int64_t> { };
        template <class _JsonType> struct AssignImpl<_JsonType, uint64_t>
            : AssignFromIntegerImpl<_JsonType, uint64_t> { };

        // 浮点数实现
        template <class _JsonType, class _Float> struct AssignFromFloatImpl {
            typedef _Float SourceType;
            static inline void invoke(_JsonType &c, SourceType arg) {
                c._valueType = _JsonType::ValueType::Float;
                c._valueFloat = static_cast<typename _JsonType::FloatType>(arg);
            }
        };

        // 浮点数
        template <class _JsonType> struct AssignImpl<_JsonType, float>
            : AssignFromFloatImpl<_JsonType, float> { };
        template <class _JsonType> struct AssignImpl<_JsonType, double>
            : AssignFromFloatImpl<_JsonType, double> { };

        // 不可移动的字符串实现
        template <class _JsonType, class _String, bool _Moveable> struct AssignFromStringImpl {
            typedef _String SourceType;
            static inline void invoke(_JsonType &c, const SourceType &arg) {
                c._valueType = _JsonType::ValueType::String;
                c._valueString = _FixString(arg);
            }
        };

        // 可移动的字符串实现
        template <class _JsonType, class _String> struct AssignFromStringImpl<_JsonType, _String, true> {
            typedef _String SourceType;
            static inline void invoke(_JsonType &c, const SourceType &arg) {
                c._valueType = _JsonType::ValueType::String;
                c._valueString = arg;
            }
            static inline void invoke(_JsonType &c, SourceType &&arg) {
                c._valueType = _JsonType::ValueType::String;
                c._valueString = std::move(arg);
            }
        };

        // C风格字符串
        template <class _JsonType, size_t _Size> struct AssignImpl<_JsonType, char [_Size]>
            : AssignFromStringImpl<_JsonType, char [_Size], false> { };
        template <class _JsonType> struct AssignImpl<_JsonType, char *>
            : AssignFromStringImpl<_JsonType, char *, false> { };
        template <class _JsonType> struct AssignImpl<_JsonType, const char *>
            : AssignFromStringImpl<_JsonType, const char *, false> { };

        // STL字符串
        template <class _JsonType, class _Traits, class _Alloc>
        struct AssignImpl<_JsonType, std::basic_string<char, _Traits, _Alloc> >
            : AssignFromStringImpl<_JsonType, std::basic_string<char, _Traits, _Alloc>,
            std::is_same<std::basic_string<char, _Traits, _Alloc>, typename _JsonType::StringType>::value> { };

        // 数组类容器迭代器
        template <class _JsonType, class Iterator>
        void _AssignFromArrayHelper(_JsonType &c, Iterator first, Iterator last) {
            c._valueType = _JsonType::ValueType::Array;
            c._child = _JsonType::New();
            _JsonType *prev = c._child;
            prev->_next = prev->_prev = prev;
            for (; first != last; ++first) {
                _JsonType *item = _JsonType::New();
                AssignImpl<_JsonType, typename std::iterator_traits<Iterator>::value_type>::invoke(*item, *first);
                prev->_next = item;
                item->_prev = prev;
                item->_next = c._child;
                c._child->_prev = item;
                ++c._child->_valueInt;
                prev = item;
            }
        }

        // 传统数组
        template <class _JsonType, class _Elem, size_t _Size>
        struct AssignImpl<_JsonType, _Elem [_Size]> {
            typedef _Elem SourceType[_Size];
            static void invoke(_JsonType &c, const SourceType &arg) {
                _AssignFromArrayHelper(c, std::begin(arg), std::end(arg));
            }
            static void invoke(_JsonType &c, SourceType &&arg) {
                _AssignFromArrayHelper(c, std::make_move_iterator(std::begin(arg)), std::make_move_iterator(std::end(arg)));
            }
        };

        // 数组类容器实现
        template <class _JsonType, class _Array>
        struct AssignFromArrayImpl {
            typedef _Array SourceType;
            static void invoke(_JsonType &c, const SourceType &arg) {
                _AssignFromArrayHelper(c, arg.begin(), arg.end());
            }
            static void invoke(_JsonType &c, SourceType &&arg) {
                _AssignFromArrayHelper(c, std::make_move_iterator(arg.begin()), std::make_move_iterator(arg.end()));
            }
        };

        // 数组类容器
        template <class _JsonType, class _T, class _Alloc>
        struct AssignImpl<_JsonType, std::vector<_T, _Alloc> >
            : AssignFromArrayImpl<_JsonType, std::vector<_T, _Alloc> > { };

        template <class _JsonType, class _T, class _Alloc>
        struct AssignImpl<_JsonType, std::list<_T, _Alloc> >
            : AssignFromArrayImpl<_JsonType, std::list<_T, _Alloc> > { };

        template <class _JsonType, class _T, class _Compare, class _Alloc>
        struct AssignImpl<_JsonType, std::set<_T, _Compare, _Alloc> >
            : AssignFromArrayImpl<_JsonType, std::set<_T, _Compare, _Alloc> > { };

        template <class _JsonType, class _T, class _Compare, class _Alloc>
        struct AssignImpl<_JsonType, std::multiset<_T, _Compare, _Alloc> >
            : AssignFromArrayImpl<_JsonType, std::multiset<_T, _Compare, _Alloc> > { };

        template <class _JsonType, class _T, class _Hash, class _Pred, class _Alloc>
        struct AssignImpl<_JsonType, std::unordered_set<_T, _Hash, _Pred, _Alloc> >
            : AssignFromArrayImpl<_JsonType, std::unordered_set<_T, _Hash, _Pred, _Alloc> > { };

        template <class _JsonType, class _T, class _Hash, class _Pred, class _Alloc>
        struct AssignImpl<_JsonType, std::unordered_multiset<_T, _Hash, _Pred, _Alloc> >
            : AssignFromArrayImpl<_JsonType, std::unordered_multiset<_T, _Hash, _Pred, _Alloc> > { };

        // 键值对类容器迭代器
        template <class _JsonType, class Iterator>
        void _AssignFromMapHelper(_JsonType &c, Iterator first, Iterator last) {
            c._valueType = _JsonType::ValueType::Object;
            c._child = _JsonType::New();
            _JsonType *prev = c._child;
            prev->_next = prev->_prev = prev;
            for (; first != last; ++first) {
                _JsonType *item = _JsonType::New();
                item->_key = _FixString((*first).first);
                AssignImpl<_JsonType, typename std::iterator_traits<Iterator>::value_type::second_type>::invoke(*item, (*first).second);
                prev->_next = item;
                item->_prev = prev;
                item->_next = c._child;
                c._child->_prev = item;
                ++c._child->_valueInt;
                prev = item;
            }
        }

        // 键值对类容器实现
        template <class _JsonType, class _Map>
        struct AssignFromMapImpl {
            typedef _Map SourceType;
            static void invoke(_JsonType &c, const SourceType &arg) {
                static_assert(std::is_convertible<const char *, typename SourceType::key_type>::value, "key_type must be able to convert to const char *");
                _AssignFromMapHelper(c, arg.begin(), arg.end());
            }

            static void invoke(_JsonType &c, SourceType &&arg) {
                static_assert(std::is_convertible<const char *, typename SourceType::key_type>::value, "key_type must be able to convert to const char *");
                _AssignFromMapHelper(c, std::make_move_iterator(arg.begin()), std::make_move_iterator(arg.end()));
            }
        };

        // 键值对类容器
        template <class _JsonType, class _Key, class _Val, class _Compare, class _Alloc>
        struct AssignImpl<_JsonType, std::map<_Key, _Val, _Compare, _Alloc> >
            : AssignFromMapImpl<_JsonType, std::map<_Key, _Val, _Compare, _Alloc> > { };

        template <class _JsonType, class _Key, class _Val, class _Compare, class _Alloc>
        struct AssignImpl<_JsonType, std::multimap<_Key, _Val, _Compare, _Alloc> >
            : AssignFromMapImpl<_JsonType, std::multimap<_Key, _Val, _Compare, _Alloc> > { };

        template <class _JsonType, class _Key, class _Val, class _Hash, class _Pred, class _Alloc>
        struct AssignImpl<_JsonType, std::unordered_map<_Key, _Val, _Hash, _Pred, _Alloc> >
            : AssignFromMapImpl<_JsonType, std::unordered_map<_Key, _Val, _Hash, _Pred, _Alloc> > { };

        template <class _JsonType, class _Key, class _Val, class _Hash, class _Pred, class _Alloc>
        struct AssignImpl<_JsonType, std::unordered_multimap<_Key, _Val, _Hash, _Pred, _Alloc> >
            : AssignFromMapImpl<_JsonType, std::unordered_multimap<_Key, _Val, _Hash, _Pred, _Alloc> > { };

        // C++11初始化列表
        template <class _JsonType, class _T>
        struct AssignImpl<_JsonType, std::initializer_list<_T> > {
            typedef std::initializer_list<_T> SourceType;
            static void invoke(_JsonType &c, const SourceType &arg) {
                // TODO:
            }
        };

        // 其他未特化的
        template <class _JsonType, class _SourceType>
        void AssignImpl<_JsonType, _SourceType>::invoke(_JsonType &c, _SourceType arg) {
            // 这里对枚举可以编译通过，对其他未特化的类型则报编译错误
            static_assert(std::is_enum<_SourceType>::value, "unimplemented type");
            AssignFromIntegerImpl<_JsonType, int64_t>::invoke(c, static_cast<int64_t>(arg));
        }

        //
        // AS
        //

        // AS成自己的指针
        template <class _JsonType>
        struct AsImpl<_JsonType, const _JsonType *> {
            typedef const _JsonType *TargetType;
            static inline TargetType invoke(const _JsonType &c) {
                return &c;
            }
        };

        // AS成自己的常引用
        template <class _JsonType>
        struct AsImpl<_JsonType, const _JsonType &> {
            typedef const _JsonType &TargetType;
            static inline TargetType invoke(const _JsonType &c) {
                return c;
            }
        };

        // AS成bool
        template <class _JsonType>
        struct AsImpl<_JsonType, bool> {
            typedef bool TargetType;
            static inline TargetType invoke(const _JsonType &c) {
                switch (c._valueType) {
                case _JsonType::ValueType::Null: return false;
                case _JsonType::ValueType::False: return false;
                case _JsonType::ValueType::True: return true;
                case _JsonType::ValueType::Integer: return !!c._valueInt;
                case _JsonType::ValueType::Float: return !!c._valueFloat;
                case _JsonType::ValueType::String: {
                    // 你非要大小写混合作死老子不伺候你！
                    if (strcmp(c._valueString.c_str(), "true") == 0 || strcmp(c._valueString.c_str(), "True") == 0
                        || strcmp(c._valueString.c_str(), "TRUE") == 0 || strcmp(c._valueString.c_str(), "1")) {
                        return true;
                    }
                    else if (strcmp(c._valueString.c_str(), "false") == 0 || strcmp(c._valueString.c_str(), "False") == 0
                        || strcmp(c._valueString.c_str(), "FALSE") == 0 || strcmp(c._valueString.c_str(), "0")) {
                        return false;
                    }
                    else {
                        throw std::logic_error("Cannot convert JSON_String to bool"); break;
                    }
                }
                case _JsonType::ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to bool"); break;
                case _JsonType::ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to bool"); break;
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        };

        // AS成整数实现
        template <class _JsonType, class _Integer>
        struct AsIntegerImpl {
            typedef _Integer TargetType;
            static TargetType invoke(const _JsonType &c) {
                switch (c._valueType) {
                case _JsonType::ValueType::Null: return TargetType(0);
                case _JsonType::ValueType::False: return TargetType(0);
                case _JsonType::ValueType::True: return TargetType(1);
                case _JsonType::ValueType::Integer: return static_cast<TargetType>(c._valueInt);
                case _JsonType::ValueType::Float: return static_cast<TargetType>(c._valueFloat);
                case _JsonType::ValueType::String: return static_cast<TargetType>(atoll(c._valueString.c_str()));
                case _JsonType::ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Integer"); break;
                case _JsonType::ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Integer"); break;
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        };

        // AS成整数
        template <class _JsonType> struct AsImpl<_JsonType, char>
            : AsIntegerImpl<_JsonType, char> { };
        template <class _JsonType> struct AsImpl<_JsonType, signed char>
            : AsIntegerImpl<_JsonType, signed char> { };
        template <class _JsonType> struct AsImpl<_JsonType, unsigned char>
            : AsIntegerImpl<_JsonType, unsigned char> { };
        template <class _JsonType> struct AsImpl<_JsonType, short>
            : AsIntegerImpl<_JsonType, short> { };
        template <class _JsonType> struct AsImpl<_JsonType, unsigned short>
            : AsIntegerImpl<_JsonType, unsigned short> { };
        template <class _JsonType> struct AsImpl<_JsonType, int>
            : AsIntegerImpl<_JsonType, int> { };
        template <class _JsonType> struct AsImpl<_JsonType, unsigned>
            : AsIntegerImpl<_JsonType, unsigned> { };
        template <class _JsonType> struct AsImpl<_JsonType, long>
            : AsIntegerImpl<_JsonType, long> { };
        template <class _JsonType> struct AsImpl<_JsonType, unsigned long>
            : AsIntegerImpl<_JsonType, unsigned long> { };
        template <class _JsonType> struct AsImpl<_JsonType, int64_t>
            : AsIntegerImpl<_JsonType, int64_t> { };
        template <class _JsonType> struct AsImpl<_JsonType, uint64_t>
            : AsIntegerImpl<_JsonType, uint64_t> { };

        // AS成浮点数实现
        template <class _JsonType, class _Float> struct AsFloatImpl {
            typedef _Float TargetType;
            static TargetType invoke(const _JsonType &c) {
                switch (c._valueType) {
                case _JsonType::ValueType::Null: return TargetType(0);
                case _JsonType::ValueType::False: return TargetType(0);
                case _JsonType::ValueType::True: return TargetType(1);
                case _JsonType::ValueType::Integer: return static_cast<TargetType>(c._valueInt);
                case _JsonType::ValueType::Float: return static_cast<TargetType>(c._valueFloat);
                case _JsonType::ValueType::String: return static_cast<TargetType>(atof(c._valueString.c_str()));
                case _JsonType::ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Float"); break;
                case _JsonType::ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Float"); break;
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        };

        // AS成浮点数
        template <class _JsonType> struct AsImpl<_JsonType, float>
            : AsFloatImpl<_JsonType, float> { };
        template <class _JsonType> struct AsImpl<_JsonType, double>
            : AsFloatImpl<_JsonType, double> { };

        // AS成STL字符串实现
        template <class _JsonType, class _String> struct AsStringImpl {
            typedef _String TargetType;
            static TargetType invoke(const _JsonType &c) {
                switch (c._valueType) {
                case _JsonType::ValueType::Null: return TargetType();
                case _JsonType::ValueType::False: return TargetType("false");
                case _JsonType::ValueType::True: return TargetType("true");
                case _JsonType::ValueType::Integer: {
                    char str[21];  // 2^64+1 can be represented in 21 chars.
                    snprintf(str, 21, "%" PRId64, (int64_t)c._valueInt);
                    return TargetType(str);
                }
                case _JsonType::ValueType::Float: {
                    std::basic_ostringstream<typename _String::value_type, typename _String::traits_type, typename _String::allocator_type> ss;
                    ss << c._valueFloat;
                    return ss.str();
                }
                case _JsonType::ValueType::String: return TargetType(c._valueString.begin(), c._valueString.end());
                case _JsonType::ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to String"); break;
                case _JsonType::ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to String"); break;
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        };

        // AS成STL字符串
        template <class _JsonType, class _Char, class _Traits, class _Alloc>
        struct AsImpl<_JsonType, std::basic_string<_Char, _Traits, _Alloc> >
            : AsStringImpl<_JsonType, std::basic_string<_Char, _Traits, _Alloc> > { };

        // AS成数组类容器实现
        template <class _JsonType, class _Array> struct AsArrayImpl {
            typedef _Array TargetType;
            static TargetType invoke(const _JsonType &c) {
                switch (c._valueType) {
                case _JsonType::ValueType::Null: return TargetType();
                case _JsonType::ValueType::False: throw std::logic_error("Cannot convert JSON_False to Array"); break;
                case _JsonType::ValueType::True: throw std::logic_error("Cannot convert JSON_True to Array"); break;
                case _JsonType::ValueType::Integer: throw std::logic_error("Cannot convert JSON_Integer to Array"); break;
                case _JsonType::ValueType::Float: throw std::logic_error("Cannot convert JSON_Float to Array"); break;
                case _JsonType::ValueType::String: throw std::logic_error("Cannot convert JSON_String to Array"); break;
                case _JsonType::ValueType::Array: {
                    TargetType ret = TargetType();
                    std::transform(c.begin(), c.end(), std::inserter(ret, ret.begin()),
                        &AsImpl<_JsonType, typename TargetType::value_type>::invoke);
                    return ret;
                }
                case _JsonType::ValueType::Object: throw std::logic_error("Cannot convert JSON_Object to Array"); break;
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        };

        // AS成数组类容器
        template <class _JsonType, class _T, class _Alloc>
        struct AsImpl<_JsonType, std::vector<_T, _Alloc> >
            : AsArrayImpl<_JsonType, std::vector<_T, _Alloc> > { };

        template <class _JsonType, class _T, class _Alloc>
        struct AsImpl<_JsonType, std::list<_T, _Alloc> >
            : AsArrayImpl<_JsonType, std::list<_T, _Alloc> > { };

        template <class _JsonType, class _T, class _Compare, class _Alloc>
        struct AsImpl<_JsonType, std::set<_T, _Compare, _Alloc> >
            : AsArrayImpl<_JsonType, std::set<_T, _Compare, _Alloc> > { };

        template <class _JsonType, class _T, class _Compare, class _Alloc>
        struct AsImpl<_JsonType, std::multiset<_T, _Compare, _Alloc> >
            : AsArrayImpl<_JsonType, std::multiset<_T, _Compare, _Alloc> > { };

        template <class _JsonType, class _T, class _Hash, class _Pred, class _Alloc>
        struct AsImpl<_JsonType, std::unordered_set<_T, _Hash, _Pred, _Alloc> >
            : AsArrayImpl<_JsonType, std::unordered_set<_T, _Hash, _Pred, _Alloc> > { };

        template <class _JsonType, class _T, class _Hash, class _Pred, class _Alloc>
        struct AsImpl<_JsonType, std::unordered_multiset<_T, _Hash, _Pred, _Alloc> >
            : AsArrayImpl<_JsonType, std::unordered_multiset<_T, _Hash, _Pred, _Alloc> > { };

        // AS成键值对类容器实现
        template <class _JsonType, class _Map> struct AsMapImpl {
            typedef _Map TargetType;
            static TargetType invoke(const _JsonType &c) {
                static_assert(std::is_convertible<const char *, typename TargetType::key_type>::value, "key_type must be able to convert to const char *");
                switch (c._valueType) {
                case _JsonType::ValueType::Null: return TargetType();
                case _JsonType::ValueType::False: throw std::logic_error("Cannot convert JSON_False to Object"); break;
                case _JsonType::ValueType::True: throw std::logic_error("Cannot convert JSON_True to Object"); break;
                case _JsonType::ValueType::Integer: throw std::logic_error("Cannot convert JSON_Integer to Object"); break;
                case _JsonType::ValueType::Float: throw std::logic_error("Cannot convert JSON_Float to Object"); break;
                case _JsonType::ValueType::String: throw std::logic_error("Cannot convert JSON_String to Object"); break;
                case _JsonType::ValueType::Array: throw std::logic_error("Cannot convert JSON_Array to Object"); break;
                case _JsonType::ValueType::Object: {
                    TargetType ret = TargetType();
                    std::transform(c.begin(), c.end(), std::inserter(ret, ret.begin()), &_make_value);
                    return ret;
                }
                default: throw std::out_of_range("JSON type out of range"); break;
                }
            }
        private:
            static inline typename TargetType::value_type _make_value(const _JsonType &j) {
                // 这里用构造第一个参数如果用typename TargetType::key_type(j._key.begin(), j._key.end())会有问题
                return typename TargetType::value_type(typename TargetType::key_type(j._key.c_str()),
                    AsImpl<_JsonType, typename TargetType::mapped_type>::invoke(j));
            }
        };

        // AS成键值对类容器
        template <class _JsonType, class _String, class _Val, class _Compare, class _Alloc>
        struct AsImpl<_JsonType, std::map<_String, _Val, _Compare, _Alloc> >
            : AsMapImpl<_JsonType, std::map<_String, _Val, _Compare, _Alloc> > { };

        template <class _JsonType, class _String, class _Val, class _Compare, class _Alloc>
        struct AsImpl<_JsonType, std::multimap<_String, _Val, _Compare, _Alloc> >
            : AsMapImpl<_JsonType, std::multimap<_String, _Val, _Compare, _Alloc> > { };

        template <class _JsonType, class _String, class _Val, class _Hash, class _Pred, class _Alloc>
        struct AsImpl<_JsonType, std::unordered_map<_String, _Val, _Hash, _Pred, _Alloc> >
            : AsMapImpl<_JsonType, std::unordered_map<_String, _Val, _Hash, _Pred, _Alloc> > { };

        template <class _JsonType, class _String, class _Val, class _Hash, class _Pred, class _Alloc>
        struct AsImpl<_JsonType, std::unordered_multimap<_String, _Val, _Hash, _Pred, _Alloc> >
            : AsMapImpl<_JsonType, std::unordered_multimap<_String, _Val, _Hash, _Pred, _Alloc> > { };

        // 其他未特化的
        template <class _JsonType, class _TargetType>
        _TargetType AsImpl<_JsonType, _TargetType>::invoke(const _JsonType &c) {
            // 这里对枚举可以编译通过，对其他未特化的类型则报编译错误
            static_assert(std::is_enum<_TargetType>::value, "unimplemented type");
            return static_cast<_TargetType>(AsImpl<_JsonType, int64_t>::invoke(c));
        }
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
