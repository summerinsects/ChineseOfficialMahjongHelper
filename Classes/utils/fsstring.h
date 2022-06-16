#ifndef __FSSTRING_H__
#define __FSSTRING_H__

#include <string.h>
#include <string>
#include <initializer_list>
#if __cplusplus >= 201703L
#include <string_view>
#define FSS_HAS_STRING_VIEW 1
#else
#define FSS_HAS_STRING_VIEW 0
#endif
#include <stdexcept>
#include <type_traits>
#include <limits>
#include <algorithm>

namespace string_utils {

#define FSS_INLINE inline
#define FSS_NOEXCEPT noexcept
#define FSS_CONSTEXPR constexpr
#if __cplusplus >= 201402L
#define FSS_CONSTEXPR_SINCE_CPP14 constexpr
#else
#define FSS_CONSTEXPR_SINCE_CPP14
#endif
#if __cplusplus >= 201703L
#define FSS_CONSTEXPR_SINCE_CPP17 constexpr
#else
#define FSS_CONSTEXPR_SINCE_CPP17
#endif
#if __cplusplus >= 202002L
#define FSS_CONSTEXPR_SINCE_CPP20 constexpr
#define FSS_NODISCARD_SINCE_CPP20 [[nodiscard]]
#else
#define FSS_CONSTEXPR_SINCE_CPP20
#define FSS_NODISCARD_SINCE_CPP20
#endif

    template <class _It>
    class _FssIt {
    public:
        typedef _It                                                             iterator_type;
        typedef typename std::iterator_traits<iterator_type>::iterator_category iterator_category;
        typedef typename std::iterator_traits<iterator_type>::value_type        value_type;
        typedef typename std::iterator_traits<iterator_type>::difference_type   difference_type;
        typedef typename std::iterator_traits<iterator_type>::pointer           pointer;
        typedef typename std::iterator_traits<iterator_type>::reference         reference;

    private:
        iterator_type _it_;

        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 _FssIt(iterator_type _x) FSS_NOEXCEPT : _it_(_x) { }

    public:
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 _FssIt() FSS_NOEXCEPT : _it_{} { }

        template <class _It2>
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 _FssIt(const _FssIt<_It2> &_other, typename std::enable_if<std::is_convertible<_It2, iterator_type>::value>::type* = 0) FSS_NOEXCEPT : _it_{_other.base()} { }

        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 _FssIt(const _FssIt &_other) : _it_(_other.base()) { }

        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 _FssIt &operator=(const _FssIt &_other) {
            if (this != &_other) { _it_ = _other._it_; }
            return *this;
        }

        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 reference operator*() const FSS_NOEXCEPT { return *_it_; }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 pointer operator->() const FSS_NOEXCEPT { return (pointer)std::addressof(*_it_); }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 _FssIt &operator++() FSS_NOEXCEPT { ++_it_; return *this; }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 _FssIt operator++(int) FSS_NOEXCEPT { _FssIt tmp(*this); ++_it_; return tmp; }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 _FssIt &operator--() FSS_NOEXCEPT { --_it_; return *this; }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 _FssIt operator--(int) FSS_NOEXCEPT { _FssIt tmp(*this); --_it_; return tmp; }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 _FssIt &operator+=(difference_type _n) FSS_NOEXCEPT { _it_ += _n; return *this; }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 _FssIt operator+(difference_type _n) const FSS_NOEXCEPT { _FssIt tmp(*this); tmp._it_ += _n; return tmp; }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 _FssIt &operator-=(difference_type _n) FSS_NOEXCEPT { _it_ -= _n; return *this; }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 _FssIt operator-(difference_type _n) const FSS_NOEXCEPT { _FssIt tmp(*this); tmp._it_ -= _n; return tmp; }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 reference operator[](difference_type _n) const FSS_NOEXCEPT { return _it_[_n]; }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 iterator_type base() const FSS_NOEXCEPT { return _it_; }

        template <class _It1> friend class _FssIt;
        template <size_t _Cap> friend struct fsstring;

        template <class _It1, class _It2>
        FSS_CONSTEXPR_SINCE_CPP17 friend bool operator==(const _FssIt<_It1> &_lhs, const _FssIt<_It2> &_rhs) FSS_NOEXCEPT;

        template <class _It1, class _It2>
        FSS_CONSTEXPR_SINCE_CPP17 friend bool operator!=(const _FssIt<_It1> &_lhs, const _FssIt<_It2> &_rhs) FSS_NOEXCEPT;

        template <class _It1, class _It2>
        FSS_CONSTEXPR_SINCE_CPP17 friend bool operator<(const _FssIt<_It1> &_lhs, const _FssIt<_It2> &_rhs) FSS_NOEXCEPT;

        template <class _It1, class _It2>
        FSS_CONSTEXPR_SINCE_CPP17 friend bool operator<=(const _FssIt<_It1> &_lhs, const _FssIt<_It2> &_rhs) FSS_NOEXCEPT;

        template <class _It1, class _It2>
        FSS_CONSTEXPR_SINCE_CPP17 friend bool operator>(const _FssIt<_It1> &_lhs, const _FssIt<_It2> &_rhs) FSS_NOEXCEPT;

        template <class _It1, class _It2>
        FSS_CONSTEXPR_SINCE_CPP17 friend bool operator>=(const _FssIt<_It1> &_lhs, const _FssIt<_It2> &_rhs) FSS_NOEXCEPT;

        template <class _It1, class _It2>
        FSS_CONSTEXPR_SINCE_CPP17 friend typename _FssIt<_It1>::difference_type operator-(const _FssIt<_It1> &_lhs, const _FssIt<_It2> &_rhs) FSS_NOEXCEPT;

        template <class _It1>
        FSS_CONSTEXPR_SINCE_CPP17 friend _FssIt<_It1> operator+(typename _FssIt<_It1>::difference_type &_n, const _FssIt<_It1> &_it) FSS_NOEXCEPT;
    };

    template <class _It1, class _It2>
    FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 bool operator==(const _FssIt<_It1> &_lhs, const _FssIt<_It2> &_rhs) FSS_NOEXCEPT {
        return _lhs._it_ == _rhs._it_;
    }

    template <class _It1, class _It2>
    FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 bool operator!=(const _FssIt<_It1> &_lhs, const _FssIt<_It2> &_rhs) FSS_NOEXCEPT {
        return _lhs._it_ != _rhs._it_;
    }

    template <class _It1, class _It2>
    FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 bool operator<(const _FssIt<_It1> &_lhs, const _FssIt<_It2> &_rhs) FSS_NOEXCEPT {
        return _lhs._it_ < _rhs._it_;
    }

    template <class _It1, class _It2>
    FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 bool operator<=(const _FssIt<_It1> &_lhs, const _FssIt<_It2> &_rhs) FSS_NOEXCEPT {
        return _lhs._it_ <= _rhs._it_;
    }

    template <class _It1, class _It2>
    FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 bool operator>(const _FssIt<_It1> &_lhs, const _FssIt<_It2> &_rhs) FSS_NOEXCEPT {
        return _lhs._it_ > _rhs._it_;
    }

    template <class _It1, class _It2>
    FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 bool operator>=(const _FssIt<_It1> &_lhs, const _FssIt<_It2> &_rhs) FSS_NOEXCEPT {
        return _lhs._it_ >= _rhs._it_;
    }

    template <class _It1, class _It2>
    FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 typename _FssIt<_It1>::difference_type operator-(const _FssIt<_It1> &_lhs, const _FssIt<_It2> &_rhs) FSS_NOEXCEPT {
        return _lhs._it_ - _rhs._it_;
    }

    template <class _It1>
    FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 _FssIt<_It1> operator+(typename _FssIt<_It1>::difference_type &_n, const _FssIt<_It1> &_it) FSS_NOEXCEPT {
        _it += _n;
        return _it;
    }

    template <size_t _Cap>
    struct fsstring {
        typedef typename std::conditional<_Cap <= std::numeric_limits<uint8_t>::max(), uint8_t,
            typename std::conditional<_Cap <= std::numeric_limits<uint16_t>::max(), uint16_t,
            typename std::conditional<_Cap <= std::numeric_limits<uint32_t>::max(), uint32_t, uint64_t>::type>::type>::type _StorageSizeT;

        typedef char                                    value_type;
        typedef value_type&                             reference;
        typedef const value_type&                       const_reference;
        typedef value_type*                             pointer;
        typedef const value_type*                       const_pointer;
        typedef size_t                                  size_type;
        typedef ptrdiff_t                               difference_type;
        typedef _FssIt<pointer>                         iterator;
        typedef _FssIt<const_pointer>                   const_iterator;
        typedef std::reverse_iterator<iterator>         reverse_iterator;
        typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;

        static constexpr size_type npos = static_cast<size_type>(-1);

        FSS_CONSTEXPR_SINCE_CPP20 fsstring() = default;
        FSS_CONSTEXPR_SINCE_CPP20 fsstring(const fsstring &other) = default;
        FSS_CONSTEXPR_SINCE_CPP20 fsstring(fsstring &&other) = default;
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &operator=(const fsstring &other) = default;
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &operator=(fsstring &&other) = default;

        FSS_CONSTEXPR_SINCE_CPP20 fsstring(size_type count, value_type ch) {
            assign(count, ch);
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring(const fsstring &other, size_type pos) {
            pos = std::min(other.size(), pos);
            assign(other.data() + pos, other.size() - pos);
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring(const fsstring &other, size_type pos, size_type count) {
            pos = std::min(other.size(), pos);
            count = std::min(other.size() - pos, count);
            assign(other.data() + pos, count);
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring(const value_type *s) {
            assign(s);
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring(const value_type *s, size_type count) {
            assign(s, count);
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring(std::initializer_list<value_type> ilist) {
            assign(&*ilist.begin(), ilist.size());
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &operator=(const value_type *s) {
            return assign(s);
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &operator=(value_type ch) {
            return assign(1, ch);
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &operator=(std::initializer_list<value_type> ilist) {
            return assign(&*ilist.begin(), ilist.size());
        }


        template <class _Traits, class _Alloc>
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &operator=(const std::basic_string<value_type, _Traits, _Alloc> &s) {
            return assign(s.data(), s.size());
        }

#if FSS_HAS_STRING_VIEW
        template <class _Traits>
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &operator=(std::basic_string_view<char, _Traits> sv) {
            return assign(sv.data(), sv.size());
        }
#endif

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &assign(const value_type *s, size_type count) {
            _size_ = static_cast<_StorageSizeT>(std::min(capacity(), count));
            memcpy(_elem_, s, _size_);
            _elem_[_size_] = '\0';
            return *this;
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &assign(size_type count, value_type ch) {
            _size_ = static_cast<_StorageSizeT>(std::min(capacity(), count));
            memset(_elem_, ch, _size_);
            _elem_[_size_] = '\0';
            return *this;
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &assign(const value_type *s) {
            return assign(s, strlen(s));
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &assign(const fsstring &other) {
            return assign(other.data(), other.size());
        }

        template <class _Traits, class _Alloc>
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &assign(const std::basic_string<value_type, _Traits, _Alloc> &s) {
            return assign(s.data(), s.size());
        }

#if FSS_HAS_STRING_VIEW
        template <class _Traits>
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &assign(std::basic_string_view<char, _Traits> sv) {
            return assign(sv.data(), sv.size());
        }

        operator std::string_view() const {
            return std::string_view(_elem_, _size_);
        }
#endif

        // iterators:
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17       iterator begin() FSS_NOEXCEPT { return iterator(data()); }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 const_iterator begin() const FSS_NOEXCEPT { return const_iterator(data()); }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17       iterator end() FSS_NOEXCEPT { return iterator(data() + _size_); }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 const_iterator end() const FSS_NOEXCEPT { return const_iterator(data() + _size_); }

        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17       reverse_iterator rbegin() FSS_NOEXCEPT { return reverse_iterator(end()); }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 const_reverse_iterator rbegin() const FSS_NOEXCEPT { return const_reverse_iterator(end()); }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17       reverse_iterator rend() FSS_NOEXCEPT { return reverse_iterator(begin()); }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 const_reverse_iterator rend() const FSS_NOEXCEPT { return const_reverse_iterator(begin()); }

        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 const_iterator cbegin() const FSS_NOEXCEPT { return begin(); }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 const_iterator cend() const FSS_NOEXCEPT { return end(); }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 const_reverse_iterator crbegin() const FSS_NOEXCEPT { return rbegin(); }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 const_reverse_iterator crend() const FSS_NOEXCEPT { return rend(); }

        // capacity:
        FSS_INLINE FSS_CONSTEXPR size_type size() const FSS_NOEXCEPT { return _size_; }
        FSS_INLINE FSS_CONSTEXPR size_type max_size() const FSS_NOEXCEPT { return _Cap - 1; }
        FSS_NODISCARD_SINCE_CPP20 FSS_INLINE FSS_CONSTEXPR bool empty() const FSS_NOEXCEPT { return _size_ == 0; }
        FSS_INLINE FSS_CONSTEXPR size_type capacity() const FSS_NOEXCEPT { return _Cap - 1; }

        // element access:
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17       reference operator[](size_type pos)       { return _elem_[pos]; }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP14 const_reference operator[](size_type pos) const { return _elem_[pos]; }

        FSS_CONSTEXPR_SINCE_CPP17       reference at(size_type pos) {
            if (pos >= _size_) throw std::out_of_range("fsstring::at");
            return _elem_[pos];
        }

        FSS_CONSTEXPR_SINCE_CPP14 const_reference at(size_type pos) const {
            if (pos >= _size_) throw std::out_of_range("fsstring::at");
            return _elem_[pos];
        }

        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17       reference front()       { return _elem_[0]; }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP14 const_reference front() const { return _elem_[0]; }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17       reference back()        { return _elem_[_size_ - 1]; }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP14 const_reference back() const  { return _elem_[_size_ - 1]; }

        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17       value_type *data()       FSS_NOEXCEPT { return _elem_; }
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 const value_type *data() const FSS_NOEXCEPT { return _elem_; }

        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP17 const value_type *c_str() const FSS_NOEXCEPT { return _elem_; }

        // operations:
        FSS_INLINE FSS_CONSTEXPR_SINCE_CPP20 void clear() FSS_NOEXCEPT { _size_ = 0; _elem_[0] = '\0'; }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &insert(size_type index, size_type count, value_type ch) {
            if (index > _size_) throw std::out_of_range("fsstring::insert");
            size_type cap = capacity();
            size_type rem = cap - _size_;
            if (rem >= count) {
                memmove(_elem_ + index + count, _elem_ + index, _size_ - index);
                memset(_elem_ + index, ch, count);
                _size_ += count;
            }
            else {
                if (index + count <= cap) {
                    memmove(_elem_ + index + count, _elem_ + index, count - rem);
                    memset(_elem_ + index, ch, count);
                }
                else {
                    memset(_elem_ + index, ch, rem);
                }
                _size_ = cap;
            }
            _elem_[_size_] = '\0';
            return *this;
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &insert(size_type index, const value_type *s, size_type count) {
            if (index > _size_) throw std::out_of_range("fsstring::insert");
            size_type cap = capacity();
            size_type rem = cap - _size_;
            if (rem >= count) {
                memmove(_elem_ + index + count, _elem_ + index, _size_ - index);
                memcpy(_elem_ + index, s, count);
                _size_ += static_cast<_StorageSizeT>(count);
            }
            else {
                if (index + count <= cap) {
                    memmove(_elem_ + index + count, _elem_ + index, count - rem);
                    memcpy(_elem_ + index, s, count);
                }
                else {
                    memcpy(_elem_ + index, s, rem);
                }
                _size_ = static_cast<_StorageSizeT>(cap);
            }
            _elem_[_size_] = '\0';
            return *this;
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &insert(size_type index, const value_type *s) {
            return insert(index, s, strlen(s));
        }

        template <class _Traits, class _Alloc>
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &insert(size_type index, const std::basic_string<value_type, _Traits, _Alloc> &s) {
            return insert(index, s.data(), s.size());
        }

        template <class _Traits, class _Alloc>
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &insert(size_type index, const std::basic_string<value_type, _Traits, _Alloc> &s, size_type index_str, size_type count = npos) {
            return insert(index, s.data() + index_str, std::min(count, s.size()));
        }

#if FSS_HAS_STRING_VIEW
        template <class _Traits>
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &insert(size_type index, std::basic_string_view<value_type, _Traits> sv) {
            return insert(index, sv.data(), sv.size());
        }

        template <class _Traits>
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &insert(size_type index, std::basic_string_view<value_type, _Traits> sv, size_type index_str, size_type count = npos) {
            return insert(index, sv.data() + index_str, std::min(count, sv.size()));
        }
#endif

        FSS_CONSTEXPR_SINCE_CPP20 iterator insert(const_iterator pos, value_type ch) {
            return insert(pos, 1, ch);
        }

        FSS_CONSTEXPR_SINCE_CPP20 iterator insert(const_iterator pos, size_type count, value_type ch) {
            difference_type n = pos - begin();
            if (count > 0) {
                insert(pos - begin(), count, ch);
            }
            return begin() + n;
        }

        FSS_CONSTEXPR_SINCE_CPP20 iterator insert(const_iterator pos, std::initializer_list<value_type> ilist) {
            difference_type n = pos - begin();
            insert(pos - begin(), ilist.begin(), ilist.size());
            return begin() + n;
        }

        template <class InputIt>
        FSS_CONSTEXPR_SINCE_CPP20 iterator insert(const_iterator pos, InputIt first, InputIt last) {
            difference_type n = pos - begin();
            insert(pos - begin(), &*first, last - first);
            return begin() + n;
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &erase(size_type index = 0, size_type count = npos) {
            size_type rem = _size_ - index;
            if (count == npos || rem <= count) {
                _size_ = index;
            }
            else {
                memmove(_elem_ + index, _elem_ + index + count, rem - count);
                _size_ -= count;
            }
            _elem_[_size_] = '\0';
            return *this;
        }

        FSS_CONSTEXPR_SINCE_CPP20 iterator erase(const_iterator pos) {
            difference_type n = pos - begin();
            erase(n, 1);
            return (n < _size_) ? begin() + n : end();
        }

        FSS_CONSTEXPR_SINCE_CPP20 iterator erase(const_iterator first, const_iterator last) {
            difference_type n = first - begin();
            difference_type count = last - first;
            erase(n, count);
            return (n < _size_) ? begin() + n : end();
        }

        FSS_CONSTEXPR_SINCE_CPP20 void push_back(value_type ch) {
            if (static_cast<size_type>(_size_) + 1u < _Cap) _elem_[_size_++] = ch, _elem_[_size_] = '\0';
        }

        FSS_CONSTEXPR_SINCE_CPP20 void pop_back() {
            if (_size_ != 0) _elem_[--_size_] = '\0';
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &append(size_type count, value_type ch) {
            count = std::min(count, capacity() - _size_);
            memset(_elem_ + _size_, ch, count);
            _size_ += count;
            _elem_[_size_] = '\0';
            return *this;
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &append(const value_type *s, size_type count) {
            count = std::min(count, capacity() - _size_);
            memcpy(_elem_ + _size_, s, count);
            _size_ += static_cast<_StorageSizeT>(count);
            _elem_[_size_] = '\0';
            return *this;
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &append(const value_type *s) {
            return append(s, strlen(s));
        }

        template <class _Traits, class _Alloc>
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &append(const std::basic_string<value_type, _Traits, _Alloc> &s) {
            return append(s.data(), s.size());
        }

#if FSS_HAS_STRING_VIEW
        template <class _Traits>
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &append(std::basic_string_view<value_type, _Traits> sv) {
            return append(sv.data(), sv.size());
        }
#endif

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &operator+=(value_type ch) {
            push_back(ch);
            return *this;
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &operator+=(const value_type *s) {
            return append(s, strlen(s));
        }

        template <class _Traits, class _Alloc>
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &operator+=(const std::basic_string<value_type, _Traits, _Alloc> &s) {
            return append(s.data(), s.size());
        }

#if FSS_HAS_STRING_VIEW
        template <class _Traits>
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &operator+=(std::basic_string_view<value_type, _Traits> sv) {
            return append(sv.data(), sv.size());
        }
#endif

        FSS_CONSTEXPR_SINCE_CPP20 int compare(const value_type *s) const {
            return compare(0, _size_, s, strlen(s));
        }

        FSS_CONSTEXPR_SINCE_CPP20 int compare(size_type pos, size_type count, const value_type *s) const {
            return compare(pos, count, s, strlen(s));
        }

        FSS_CONSTEXPR_SINCE_CPP20 int compare(size_type pos1, size_type count1, const value_type *s, size_type count2) const {
            count1 = std::min<size_type>(count1, _size_);
            if (count1 < count2) return -1;
            if (count1 > count2) return 1;
            return memcmp(_elem_ + pos1, s, count1);
        }

        FSS_CONSTEXPR_SINCE_CPP20 int compare(const fsstring &s) const {
            return compare(0, _size_, s.data(), s.size());
        }

        template <class _Traits, class _Alloc>
        FSS_CONSTEXPR_SINCE_CPP20 int compare(const std::basic_string<value_type, _Traits, _Alloc> &s) const { return compare(0, _size_, s.data(), s.size()); }

#if FSS_HAS_STRING_VIEW
        template <class _Traits>
        FSS_CONSTEXPR_SINCE_CPP20 int compare(std::basic_string_view<value_type, _Traits> sv) const { return compare(0, _size_, sv.data(), sv.size()); }
#endif

#if __cplusplus >= 202002L
        template <class _Traits>
        constexpr bool starts_with(std::basic_string_view<value_type, _Traits> sv) const noexcept {
            auto sz = sv.size();
            return _size_ >= sz && compare(0, sz, sv.data());
        }

        constexpr bool starts_with(value_type ch) const noexcept {
            return !empty() && _elem_[0] == ch;
        }

        constexpr bool starts_with(const value_type *s) const {
            return starts_with(std::string_view(s));
        }

        template <class _Traits>
        constexpr bool ends_with(std::basic_string_view<value_type, _Traits> sv) const noexcept {
            auto sz = sv.size();
            return _size_ >= sz && compare(_size_ - sz, sz, sv.data());
        }

        constexpr bool ends_with(value_type ch) const noexcept {
            return !empty() && _elem_[_size_ - 1] == ch;
        }

        constexpr bool ends_with(const value_type *s) const {
            return ends_with(std::string_view(s));
        }
#endif

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &replace(size_type pos, size_type count, size_type count2, value_type ch) {
            pos = std::min<size_type>(pos, _size_ - 1);
            count = std::min<size_type>(count, _size_ - pos);
            if (count == count2) {
                memset(_elem_ + pos, ch, count);
            }
            else if (count > count2) {
                size_type rem = count - count2;
                memset(_elem_ + pos, ch, count2);
                memmove(_elem_ + pos + count2, _elem_ + pos + count, _size_ - pos - count);
                _size_ -= static_cast<_StorageSizeT>(rem);
                _elem_[_size_] = '\0';
            }
            else {
                size_type rem = count2 - count;
                memset(_elem_ + pos, ch, count);
                insert(pos + count, rem, ch);
            }
            return *this;
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &replace(size_type pos, size_type count, const value_type *cstr, size_type count2) {
            pos = std::min<size_type>(pos, _size_ - 1);
            count = std::min<size_type>(count, _size_ - pos);
            if (count == count2) {
                memcpy(_elem_ + pos, cstr, count);
            }
            else if (count > count2) {
                size_type rem = count - count2;
                memcpy(_elem_ + pos, cstr, count2);
                memmove(_elem_ + pos + count2, _elem_ + pos + count, _size_ - pos - count);
                _size_ -= static_cast<_StorageSizeT>(rem);
                _elem_[_size_] = '\0';
            }
            else {
                size_type rem = count2 - count;
                memcpy(_elem_ + pos, cstr, count);
                insert(pos + count, cstr + count, rem);
            }
            return *this;
        }

        FSS_CONSTEXPR_SINCE_CPP20 fsstring &replace(size_type pos, size_type count, const value_type *cstr) {
            return replace(pos, count, cstr, strlen(cstr));
        }

        template <class _Traits, class _Alloc>
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &replace(size_type pos, size_type count, const std::basic_string<value_type, _Traits, _Alloc> &s) {
            return replace(pos, count, s.data(), s.size());
        }

#if FSS_HAS_STRING_VIEW
        template <class _Traits>
        FSS_CONSTEXPR_SINCE_CPP20 fsstring &replace(size_type pos, size_type count, std::basic_string_view<value_type, _Traits> sv) {
            return replace(pos, count, sv.data(), sv.size());
        }
#endif

        FSS_CONSTEXPR_SINCE_CPP20 void resize(size_type count, value_type ch) {
            if (count > max_size()) throw std::length_error("fsstring::resize");
            if (count <= _size_) {
                _size_ = count;
            }
            else {
                memset(_elem_ + _size_, ch, count - _size_);
                _size_ += count;
            }
            _elem_[_size_] = '\0';
        }

        FSS_CONSTEXPR_SINCE_CPP20 void resize(size_type count) {
            resize(count, '\0');
        }

        FSS_CONSTEXPR_SINCE_CPP20 void swap(fsstring &other) noexcept {
            std::swap(_elem_, other._elem_);
            std::swap(_size_, other._size_);
        }

        FSS_CONSTEXPR_SINCE_CPP20 size_type find(value_type ch, size_type pos = 0) const noexcept {
            if (pos >= _size_) return npos;
            auto res = reinterpret_cast<const char *>(memchr(_elem_ + pos, ch, _size_ - pos));
            return res ? static_cast<size_type>(res - _elem_) : npos;
        }

        FSS_CONSTEXPR_SINCE_CPP20 size_type find(const value_type *s, size_type pos, size_type count) const {
            if (count == 0) return pos;
            if (count > _size_) return npos;
            do {
                pos = find(*s, pos);
                if (pos == npos || _size_ - pos < count) return npos;
                if (0 == memcmp(_elem_ + pos, s, count)) return pos;
                ++pos;
            } while (1);
        }

        FSS_CONSTEXPR_SINCE_CPP20 size_type find(const value_type *s, size_type pos = 0) const {
            return find(s, pos, strlen(s));
        }

        template <class _Traits, class _Alloc>
        FSS_CONSTEXPR_SINCE_CPP20 size_type find(const std::basic_string<value_type, _Traits, _Alloc> &s, size_type pos = 0) const {
            return find(s.data(), pos, s.size());
        }

#if FSS_HAS_STRING_VIEW
        template <class _Traits>
        FSS_CONSTEXPR_SINCE_CPP20 size_type find(std::basic_string_view<value_type, _Traits> sv, size_type pos = 0) const {
            return find(sv.data(), pos, sv.size());
        }
#endif

        FSS_CONSTEXPR_SINCE_CPP20 size_type rfind(const value_type *s, size_type pos, size_type count) const {
            pos = std::min<size_type>(pos, _size_);
            if (count < _size_ - pos) pos += count;
            else pos = _size_;
            const value_type *res = std::find_end(_elem_, _elem_ + pos, s, s + count);
            if (count > 0 && res == _elem_ + pos) return npos;
            return static_cast<size_type>(res - _elem_);
        }

        FSS_CONSTEXPR_SINCE_CPP20 size_type rfind(const value_type *s, size_type pos = npos) const {
            return rfind(s, pos, strlen(s));
        }

        FSS_CONSTEXPR_SINCE_CPP20 size_type rfind(value_type ch, size_type pos = npos) const noexcept {
            if (_size_ < 1) return npos;

            if (pos < _size_) ++pos;
            else pos = _size_;

            for (const value_type *p = _elem_ + pos; p != _elem_; ) {
                if (*--p == ch) {
                     return static_cast<size_type>(p - _elem_);
                }
            }
            return npos;
        }

        template <class _Traits, class _Alloc>
        FSS_CONSTEXPR_SINCE_CPP20 size_type rfind(const std::basic_string<value_type, _Traits, _Alloc> &s, size_type pos = 0) const {
            return rfind(s.data(), pos, s.size());
        }

#if FSS_HAS_STRING_VIEW
        template <class _Traits>
        FSS_CONSTEXPR_SINCE_CPP20 size_type rfind(std::basic_string_view<value_type, _Traits> sv, size_type pos = 0) const {
            return rfind(sv.data(), pos, sv.size());
        }
#endif

        FSS_CONSTEXPR_SINCE_CPP20 size_type find_first_of(const value_type *s, size_type pos, size_type count) const {
            auto res = std::find_first_of(_elem_ + pos, _elem_ + _size_, s, s + count);
            return (res != _elem_ + _size_) ? static_cast<size_type>(res - _elem_) : npos;
        }

        FSS_CONSTEXPR_SINCE_CPP20 size_type find_first_of(const value_type *s, size_type pos = 0) const {
            return find_first_of(s, pos, strlen(s));
        }

        FSS_CONSTEXPR_SINCE_CPP20 size_type find_first_of(value_type ch, size_type pos = 0) const noexcept {
            return find(ch, pos);
        }

        template <class _Traits, class _Alloc>
        FSS_CONSTEXPR_SINCE_CPP20 size_type find_first_of(const std::basic_string<value_type, _Traits, _Alloc> &s, size_type pos = 0) const {
            return find_first_of(s.data(), pos, s.size());
        }

#if FSS_HAS_STRING_VIEW
        template <class _Traits>
        FSS_CONSTEXPR_SINCE_CPP20 size_type find_first_of(std::basic_string_view<value_type, _Traits> sv, size_type pos = 0) const {
            return find_first_of(sv.data(), pos, sv.size());
        }
#endif

        FSS_CONSTEXPR_SINCE_CPP20 size_type find_first_not_of(const value_type *s, size_type pos, size_type count) const {
            if (pos < _size_) {
                const value_type *pe = _elem_ + _size_;
                for (const value_type *p = _elem_ + pos; p != pe; ++p) {
                    if (memchr(s, *p, count) == nullptr) {
                        return static_cast<size_type>(p - _elem_);
                    }
                }
            }
            return npos;
        }

        FSS_CONSTEXPR_SINCE_CPP20 size_type find_first_not_of(const value_type *s, size_type pos = 0) const {
            return find_first_not_of(s, pos, strlen(s));
        }

        FSS_CONSTEXPR_SINCE_CPP20 size_type find_first_not_of(value_type ch, size_type pos = 0) const noexcept {
            if (pos < _size_) {
                const value_type *pe = _elem_ + _size_;
                for (const value_type *p = _elem_ + pos; p != pe; ++p) {
                    if (*p != ch) {
                        return static_cast<size_type>(p - _elem_); 
                    }
                }
            }
            return npos;
        }

        template <class _Traits, class _Alloc>
        FSS_CONSTEXPR_SINCE_CPP20 size_type find_first_not_of(const std::basic_string<value_type, _Traits, _Alloc> &s, size_type pos = 0) const {
            return find_first_not_of(s.data(), pos, s.size());
        }

#if FSS_HAS_STRING_VIEW
        template <class _Traits>
        FSS_CONSTEXPR_SINCE_CPP20 size_type find_first_not_of(std::basic_string_view<value_type, _Traits> sv, size_type pos = 0) const {
            return find_first_not_of(sv.data(), pos, sv.size());
        }
#endif

        FSS_CONSTEXPR_SINCE_CPP20 size_type find_last_of(const value_type* s, size_type pos, size_type count) const {
            if (count != 0) {
                if (pos < _size_) ++pos;
                else pos = _size_;

                for (const value_type *p = _elem_ + pos; p != _elem_; ) {
                    if (memchr(s, *--p, count)) {
                        return static_cast<size_type>(p - _elem_);
                    }
                }
            }
            return npos;
        }

        FSS_CONSTEXPR_SINCE_CPP20 size_type find_last_of(const value_type* s, size_type pos = npos) const {
            return find_last_of(s, pos, strlen(s));
        }

        FSS_CONSTEXPR_SINCE_CPP20 size_type find_last_of(value_type ch, size_type pos = npos) const noexcept {
            return rfind(ch, pos);
        }

        template <class _Traits, class _Alloc>
        FSS_CONSTEXPR_SINCE_CPP20 size_type find_last_of(const std::basic_string<value_type, _Traits, _Alloc> &s, size_type pos = 0) const {
            return find_last_of(s.data(), pos, s.size());
        }

#if FSS_HAS_STRING_VIEW
        template <class _Traits>
        FSS_CONSTEXPR_SINCE_CPP20 size_type find_last_of(std::basic_string_view<value_type, _Traits> sv, size_type pos = 0) const {
            return find_last_of(sv.data(), pos, sv.size());
        }
#endif

        FSS_CONSTEXPR_SINCE_CPP20 size_type find_last_not_of(const value_type *s, size_type pos, size_type count) const {
            if (pos < _size_) ++pos;
            else pos = _size_;

            for (const value_type *p = _elem_ + pos; p != _elem_; ) {
                if (memchr(s, *--p, count) == nullptr) {
                    return static_cast<size_type>(p - _elem_);
                }
            }
            return npos;
        }

        FSS_CONSTEXPR_SINCE_CPP20 size_type find_last_not_of(const value_type *s, size_type pos = npos) const {
            return find_last_not_of(s, pos, strlen(s));
        }

        FSS_CONSTEXPR_SINCE_CPP20 size_type find_last_not_of(value_type ch, size_type pos = npos) const noexcept {
            if (pos < _size_) ++pos;
            else pos = _size_;

            for (const value_type *p = _elem_ + pos; p != _elem_; ) {
                if (*--p != ch) {
                    return static_cast<size_type>(p - _elem_);
                }
            }
            return npos;
        }

        template <class _Traits, class _Alloc>
        FSS_CONSTEXPR_SINCE_CPP20 size_type find_last_not_of(const std::basic_string<value_type, _Traits, _Alloc> &s, size_type pos = 0) const {
            return find_last_not_of(s.data(), pos, s.size());
        }

#if FSS_HAS_STRING_VIEW
        template <class _Traits>
        FSS_CONSTEXPR_SINCE_CPP20 size_type find_last_not_of(std::basic_string_view<value_type, _Traits> sv, size_type pos = 0) const {
            return find_last_not_of(sv.data(), pos, sv.size());
        }
#endif

        value_type _elem_[_Cap];
        _StorageSizeT _size_;
    };

    template <size_t _Cap>
    FSS_INLINE bool operator==(const fsstring<_Cap> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        auto sz = lhs.size();
        return sz == rhs.size() && 0 == lhs.compare(0, sz, rhs.data());
    }

    template <size_t _Cap>
    FSS_INLINE bool operator==(const char *lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return 0 == rhs.compare(lhs);
    }

    template <size_t _Cap>
    FSS_INLINE bool operator==(const fsstring<_Cap> &lhs, const char *rhs) FSS_NOEXCEPT {
        return rhs == lhs;
    }

    template <size_t _Cap, class _Traits, class _Alloc>
    FSS_INLINE bool operator==(const fsstring<_Cap> &lhs, const std::basic_string<char, _Traits, _Alloc> &rhs) FSS_NOEXCEPT {
        auto sz = lhs.size();
        return sz == rhs.size() && 0 == lhs.compare(0, sz, rhs.data());
    }

    template <size_t _Cap, class _Traits, class _Alloc>
    FSS_INLINE bool operator==(const std::basic_string<char, _Traits, _Alloc> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return rhs == lhs;
    }

#if FSS_HAS_STRING_VIEW
    template <size_t _Cap, class _Traits>
    FSS_INLINE bool operator==(const fsstring<_Cap> &lhs, const std::basic_string_view<char, _Traits> &rhs) FSS_NOEXCEPT {
        auto sz = lhs.size();
        return sz == rhs.size() && 0 == lhs.compare(0, sz, rhs.data());
    }

    template <size_t _Cap, class _Traits>
    FSS_INLINE bool operator==(const std::basic_string_view<char, _Traits> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return rhs == lhs;
    }
#endif

    template <size_t _Cap>
    FSS_INLINE bool operator!=(const fsstring<_Cap> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return !(lhs == rhs);
    }

    template <size_t _Cap>
    FSS_INLINE bool operator!=(const char *lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return !(lhs == rhs);
    }

    template <size_t _Cap>
    FSS_INLINE bool operator!=(const fsstring<_Cap> &lhs, const char *rhs) FSS_NOEXCEPT {
        return !(rhs == lhs);
    }

    template <size_t _Cap, class _Traits, class _Alloc>
    FSS_INLINE bool operator!=(const fsstring<_Cap> &lhs, const std::basic_string<char, _Traits, _Alloc> &rhs) FSS_NOEXCEPT {
        return !(lhs == rhs);
    }

    template <size_t _Cap, class _Traits, class _Alloc>
    FSS_INLINE bool operator!=(const std::basic_string<char, _Traits, _Alloc> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return !(rhs == lhs);
    }

#if FSS_HAS_STRING_VIEW
    template <size_t _Cap, class _Traits>
    FSS_INLINE bool operator!=(const fsstring<_Cap> &lhs, const std::basic_string_view<char, _Traits> &rhs) FSS_NOEXCEPT {
        return !(lhs == rhs);
    }

    template <size_t _Cap, class _Traits>
    FSS_INLINE bool operator!=(const std::basic_string_view<char, _Traits> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return !(rhs == lhs);
    }
#endif

    template <size_t _Cap>
    FSS_INLINE bool operator<(const fsstring<_Cap> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return lhs.compare(rhs) < 0;
    }

    template <size_t _Cap>
    FSS_INLINE bool operator<(const char *lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return rhs.compare(lhs) > 0;
    }

    template <size_t _Cap>
    FSS_INLINE bool operator<(const fsstring<_Cap> &lhs, const char *rhs) FSS_NOEXCEPT {
        return lhs.compare(rhs) < 0;
    }

    template <size_t _Cap, class _Traits, class _Alloc>
    FSS_INLINE bool operator<(const fsstring<_Cap> &lhs, const std::basic_string<char, _Traits, _Alloc> &rhs) FSS_NOEXCEPT {
        return lhs.compare(rhs) < 0;
    }

    template <size_t _Cap, class _Traits, class _Alloc>
    FSS_INLINE bool operator<(const std::basic_string<char, _Traits, _Alloc> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return rhs.compare(lhs) > 0;
    }

#if FSS_HAS_STRING_VIEW
    template <size_t _Cap, class _Traits>
    FSS_INLINE bool operator<(const fsstring<_Cap> &lhs, const std::basic_string_view<char, _Traits> &rhs) FSS_NOEXCEPT {
        return lhs.compare(rhs) < 0;
    }

    template <size_t _Cap, class _Traits>
    FSS_INLINE bool operator<(const std::basic_string_view<char, _Traits> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return rhs.compare(lhs) > 0;
    }
#endif

    template <size_t _Cap>
    FSS_INLINE bool operator>(const fsstring<_Cap> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return rhs < lhs;
    }

    template <size_t _Cap>
    FSS_INLINE bool operator>(const char *lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return rhs < lhs;
    }

    template <size_t _Cap>
    FSS_INLINE bool operator>(const fsstring<_Cap> &lhs, const char *rhs) FSS_NOEXCEPT {
        return rhs < lhs;
    }

    template <size_t _Cap, class _Traits, class _Alloc>
    FSS_INLINE bool operator>(const fsstring<_Cap> &lhs, const std::basic_string<char, _Traits, _Alloc> &rhs) FSS_NOEXCEPT {
        return rhs < lhs;
    }

    template <size_t _Cap, class _Traits, class _Alloc>
    FSS_INLINE bool operator>(const std::basic_string<char, _Traits, _Alloc> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return rhs < lhs;
    }

#if FSS_HAS_STRING_VIEW
    template <size_t _Cap, class _Traits>
    FSS_INLINE bool operator>(const fsstring<_Cap> &lhs, const std::basic_string_view<char, _Traits> &rhs) FSS_NOEXCEPT {
        return rhs < lhs;
    }

    template <size_t _Cap, class _Traits>
    FSS_INLINE bool operator>(const std::basic_string_view<char, _Traits> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return rhs < lhs;
    }
#endif

    template <size_t _Cap>
    FSS_INLINE bool operator<=(const fsstring<_Cap> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return !(rhs < lhs);
    }

    template <size_t _Cap>
    FSS_INLINE bool operator<=(const char *lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return !(rhs < lhs);
    }

    template <size_t _Cap>
    FSS_INLINE bool operator<=(const fsstring<_Cap> &lhs, const char *rhs) FSS_NOEXCEPT {
        return !(rhs < lhs);
    }

    template <size_t _Cap, class _Traits, class _Alloc>
    FSS_INLINE bool operator<=(const fsstring<_Cap> &lhs, const std::basic_string<char, _Traits, _Alloc> &rhs) FSS_NOEXCEPT {
        return !(rhs < lhs);
    }

    template <size_t _Cap, class _Traits, class _Alloc>
    FSS_INLINE bool operator<=(const std::basic_string<char, _Traits, _Alloc> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return !(rhs < lhs);
    }

#if FSS_HAS_STRING_VIEW
    template <size_t _Cap, class _Traits>
    FSS_INLINE bool operator<=(const fsstring<_Cap> &lhs, const std::basic_string_view<char, _Traits> &rhs) FSS_NOEXCEPT {
        return !(rhs < lhs);
    }

    template <size_t _Cap, class _Traits>
    FSS_INLINE bool operator<=(const std::basic_string_view<char, _Traits> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return !(rhs < lhs);
    }
#endif

    template <size_t _Cap>
    FSS_INLINE bool operator>=(const fsstring<_Cap> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return !(lhs < rhs);
    }

    template <size_t _Cap>
    FSS_INLINE bool operator>=(const char *lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return !(lhs < rhs);
    }

    template <size_t _Cap>
    FSS_INLINE bool operator>=(const fsstring<_Cap> &lhs, const char *rhs) FSS_NOEXCEPT {
        return !(lhs < rhs);
    }

    template <size_t _Cap, class _Traits, class _Alloc>
    FSS_INLINE bool operator>=(const fsstring<_Cap> &lhs, const std::basic_string<char, _Traits, _Alloc> &rhs) FSS_NOEXCEPT {
        return !(lhs < rhs);
    }

    template <size_t _Cap, class _Traits, class _Alloc>
    FSS_INLINE bool operator>=(const std::basic_string<char, _Traits, _Alloc> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return !(lhs < rhs);
    }

#if FSS_HAS_STRING_VIEW
    template <size_t _Cap, class _Traits>
    FSS_INLINE bool operator>=(const fsstring<_Cap> &lhs, const std::basic_string_view<char, _Traits> &rhs) FSS_NOEXCEPT {
        return !(lhs < rhs);
    }

    template <size_t _Cap, class _Traits>
    FSS_INLINE bool operator>=(const std::basic_string_view<char, _Traits> &lhs, const fsstring<_Cap> &rhs) FSS_NOEXCEPT {
        return !(lhs < rhs);
    }
#endif

#undef FSS_INLINE
#undef FSS_NOEXCEPT
#undef FSS_CONSTEXPR
#undef FSS_CONSTEXPR_SINCE_CPP14
#undef FSS_CONSTEXPR_SINCE_CPP17
#undef FSS_CONSTEXPR_SINCE_CPP20
#undef FSS_NODISCARD_SINCE_CPP20

}

#endif
