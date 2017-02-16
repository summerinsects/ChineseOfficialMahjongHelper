//#if (defined _DEBUG) || (defined DEBUG)
//#   define CRTDBG_MAP_ALLOC
//#   include <crtdbg.h>
//#endif

#include "cppJSON.hpp"

#include <iostream>

template <class _T> void func(const _T &t)
{
    for (auto &v : t) {
        std::cout << v << std::endl;
    }
    std::cout << std::endl;
}

template <class _MAP> void func2(const _MAP &map) {
    for (auto &v : map) {
        std::cout << v.first << '\t' << v.second << std::endl;
    }
    std::cout << std::endl;
}

struct TestCharTraits {
    typedef char _Elem;
    typedef _Elem char_type;
    typedef int int_type;
    typedef std::streampos pos_type;
    typedef std::streamoff off_type;
    typedef int state_type;

    static int compare(const _Elem *_First1, const _Elem *_First2,
        size_t _Count) {
        return (_Count == 0 ? 0 : memcmp(_First1, _First2, _Count));
    }

    static size_t length(const _Elem *_First) {	// find length of null-terminated string
        return (*_First == 0 ? 0 : strlen(_First));
    }

    static _Elem *copy(_Elem *_First1, const _Elem *_First2, size_t _Count) {	// copy [_First2, _First2 + _Count) to [_First1, ...)
        return (_Count == 0 ? _First1 : (_Elem *)memcpy(_First1, _First2, _Count));
    }

    //static _Elem *_Copy_s(
    //    _Out_writes_(_Size_in_bytes) _Elem *_First1, size_t _Size_in_bytes,
    //    _In_reads_(_Count) const _Elem *_First2, size_t _Count)
    //{	// copy [_First2, _First2 + _Count) to [_First1, ...)
    //    if (0 < _Count)
    //        _CRT_SECURE_MEMCPY(_First1, _Size_in_bytes, _First2, _Count);
    //    return (_First1);
    //}

    static const _Elem * find(const _Elem *_First, size_t _Count, const _Elem& _Ch) {	// look for _Ch in [_First, _First + _Count)
        return (_Count == 0 ? (const _Elem *)0 : (const _Elem *)memchr(_First, _Ch, _Count));
    }

    static _Elem * move(_Elem *_First1, const _Elem *_First2, size_t _Count) {	// copy [_First2, _First2 + _Count) to [_First1, ...)
        return (_Count == 0 ? _First1 : (_Elem *)memmove(_First1, _First2, _Count));
    }

    static _Elem *assign(_Elem *_First, size_t _Count, _Elem _Ch) {	// assign _Count * _Ch to [_First, ...)
        return ((_Elem *)memset(_First, _Ch, _Count));
    }

    static void assign(_Elem& _Left, const _Elem& _Right) throw() {	// assign an element
        _Left = _Right;
    }

    static bool eq(const _Elem& _Left, const _Elem& _Right) throw() {	// test for element equality
        return (_Left == _Right);
    }
    
    static bool lt(const _Elem& _Left, const _Elem& _Right) throw() {	// test if _Left precedes _Right
        return ((unsigned char)_Left < (unsigned char)_Right);
    }

    static _Elem to_char_type(const int_type& _Meta) throw() {	// convert metacharacter to character
        return ((_Elem)_Meta);
    }

    static int_type to_int_type(const _Elem& _Ch) throw() {	// convert character to metacharacter
        return ((unsigned char)_Ch);
    }

    static bool eq_int_type(const int_type& _Left, const int_type& _Right) throw() {	// test for metacharacter equality
        return (_Left == _Right);
    }

    static int_type not_eof(const int_type& _Meta) throw() {	// return anything but EOF
        return (_Meta != eof() ? _Meta : !eof());
    }

    static int_type eof() throw() {	// return end-of-file metacharacter
        return (EOF);
    }
};

//#include <windows.h>
#undef max

template <class _T> struct TestAllocator {
    // typedefs
    typedef _T                  value_type;
    typedef value_type          *pointer;
    typedef value_type          &reference;
    typedef const value_type    *const_pointer;
    typedef const value_type    &const_reference;
    typedef size_t              size_type;
    typedef ptrdiff_t           difference_type;

    // rebind
    template <class _Other> struct rebind {
        typedef TestAllocator<_Other> other;
    };

    pointer address(reference val) const {
        return &val;
    }

    const_pointer address(const_reference val) const {
        return &val;
    }

    // construct default allocator (do nothing)
    TestAllocator() throw() {
    }

    // construct by copying (do nothing)
    TestAllocator(const TestAllocator<_T> &) throw() {
    }

    // construct from a related allocator (do nothing)
    template<class _Other>
    TestAllocator(const TestAllocator<_Other> &) throw() {
    }

    // assign from a related allocator (do nothing)
    template<class _Other>
    TestAllocator<_T> &operator=(const TestAllocator<_Other> &) {
        return *this;
    }

    // deallocate object at _Ptr, ignore size
    void deallocate(pointer ptr, size_type) {
        //delete ptr;
        //free(ptr);
        //::HeapFree(::GetProcessHeap(), 0, ptr);
        free(ptr);
    }

    // allocate array of cnt elements
    pointer allocate(size_type cnt) {
        //return _Allocate(cnt, (pointer)0);
        //return calloc(cnt, sizeof(_T));
        //return (pointer)::HeapAlloc(::GetProcessHeap(), 0, cnt * sizeof(_T));
        return (pointer)malloc(cnt * sizeof(_T));
    }

    // allocate array of cnt elements, ignore hint
    pointer allocate(size_type cnt, const void *) {
        return allocate(cnt);
    }

    // default construct object at ptr
    void construct(_T *ptr) {
        if (ptr == nullptr) throw std::invalid_argument("");
        new ((void *)ptr) _T();
    }

    // construct object at ptr with value val
    void construct(_T *ptr, const _T &val) {
        if (ptr == nullptr) throw std::invalid_argument("");
        new ((void *)ptr) _T(val);
    }

    // construct _Obj(_Types...) at ptr
    template <class _Obj, class... _Types>
    void construct(_Obj *ptr, _Types &&...args) {
        if (ptr == nullptr) throw std::invalid_argument("");
        new ((void *)ptr) _Obj(std::forward<_Types>(args)...);
    }

    // destroy object at ptr
    template<class _U>
    void destroy(_U *ptr) {
        ptr->~_U();
    }

    // estimate maximum array size
    size_t max_size() const throw() {
        return ((size_t)(-1) / sizeof(_T));
    }
};

// test for allocator equality
template <class _T, class _Other>
inline bool operator==(const TestAllocator<_T> &, const TestAllocator<_Other> &) throw() {
    return true;
}

// test for allocator inequality
template <class _T, class _Other>
inline bool operator!=(const TestAllocator<_T> &left, const TestAllocator<_Other> &right) throw() {
    return !(left == right);
}

#include <algorithm>
#include <functional>
#include <stdbool.h>

#include <stack>
#include <queue>

enum E1 {
    E1_Value
};

enum class E2 {
    E2_Value
};

int main(int argc, char *argv[])
{
//#if (defined _DEBUG) || (defined DEBUG)
//    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//    //_CrtSetBreakAlloc(147);
//#endif

    typedef jw::BasicJSON<int, float, TestCharTraits, TestAllocator<char> > cppJSON;

	std::cout << "==========Construct From BasicTypes==========" << std::endl;
	std::cout << cppJSON() << std::endl;
	std::cout << cppJSON(nullptr) << std::endl;
	std::cout << cppJSON(true) << std::endl;
	std::cout << cppJSON(false) << std::endl;
	std::cout << cppJSON(std::numeric_limits<unsigned>::max()) << std::endl;
	std::cout << cppJSON(std::numeric_limits<int64_t>::max()) << std::endl;
	std::cout << cppJSON(0.5F) << std::endl;
	std::cout << cppJSON(3.14) << std::endl;

	std::cout << "==========Construct From String==========" << std::endl;
	std::cout << cppJSON("abcd") << std::endl;
	const char *str1 = "qwer";
	std::cout << cppJSON(str1) << std::endl;
	const char str2[] = "abcd";
	std::cout << cppJSON(str2) << std::endl;
	char str3[] = "zxcv";
	std::cout << cppJSON(str3) << std::endl;
	char *str4 = str3;
	std::cout << cppJSON(str4) << std::endl;
	std::cout << cppJSON(std::string("123")) << std::endl;
    std::cout << cppJSON(cppJSON::StringType("567")) << std::endl;

	std::cout << "==========Construct From Array==========" << std::endl;
	std::cout << cppJSON(std::vector<int>({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 })) << std::endl;
	std::cout << cppJSON(std::list<double>({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 })) << std::endl;
    std::cout << cppJSON(std::forward_list<float>({ 1, 2, 3, 4, 5 })) << std::endl;
    std::cout << cppJSON(std::set<unsigned>({ '1', '2', '3', '4', '5' })) << std::endl;  // TODO
    std::cout << cppJSON(std::multiset<float>({ 1, 2, 3, 4, 5, 1 })) << std::endl;
    std::cout << cppJSON(std::unordered_set<unsigned>({ '1', '2', '3', '4', '5' })) << std::endl;
    std::cout << cppJSON(std::unordered_multiset<float>({ 1, 2, 3, 4, 5, 1 })) << std::endl;
    std::cout << cppJSON({1, 2, 3, 4, 5}) << std::endl;
    std::cout << cppJSON(std::deque<unsigned>({ 1, 2, 3, 4, 5, 7 })) << std::endl;
    std::cout << cppJSON(std::array<int, 5>()) << std::endl;

    const float arr[] = { 0, 1, 2, 3, 4, 5 };
	std::cout << cppJSON(arr) << std::endl;
	unsigned uarr[] = { 10, 9, 8, 7, 6 };
	std::cout << cppJSON(uarr) << std::endl;
    //std::cout << cppJSON(std::move(uarr)) << std::endl;
    std::vector<cppJSON::StringType> sarr = {"abc", "def", "ghi"};
    std::cout << cppJSON(sarr) << std::endl;
    std::cout << cppJSON(std::move(sarr)) << std::endl;

    std::cout << cppJSON({ 1, 2, 3, 4, 5 }) << std::endl;
	auto initList = { "1", "2", "3", "c" };
	std::cout << cppJSON(initList) << std::endl;

    std::cout << "==========Construct From Map==========" << std::endl;
    std::cout << cppJSON(std::map<const char *, int>({ { "a", 1 }, { "b", 2 }, { "c", 3 }, { "d", 4 }, { "e", 5 } })) << std::endl;
    std::cout << cppJSON(std::multimap<cppJSON::StringType, cppJSON::StringType>({ { "a", "a" }, { "b", "b" }, { "c", "c" }, { "d", "d" }, { "e", "e" } })) << std::endl;
    std::cout << cppJSON(std::unordered_map<std::string, int>({ { "a", 1 }, { "b", 2 }, { "c", 3 }, { "d", 4 }, { "e", 5 } })) << std::endl;
	// TODO:
	//std::wcout << js5.as<std::wstring>();// << std::endl;

	//std::cout << "==========Construct From Array End==========" << std::endl;

    std::cout << "==========Parse Array==========" << std::endl;
    cppJSON json;
    json.Parse("[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]");
    std::cout << json.stringfiy() << std::endl;
    std::cout << "==========As Array==========" << std::endl;
    func(json.as<std::vector<std::string> >());
    func(json.as<std::list<std::string> >());
    func(json.as<std::set<std::string> >());
    func(json.as<std::multiset<std::string> >());
    func(json.as<std::unordered_set<std::string> >());
    func(json.as<std::unordered_multiset<std::string> >());
    func(json.as<std::deque<std::string> >());
    //json.clear();

    std::cout << "==========Parse Map==========" << std::endl;
    json.Parse("{\"name2\":\"Jack\",\"name3\":\"Jack\",\"name4\":\"Jack\",\"name5\":\"Jack\",\"name6\":\"Jack\",\"name7\":\"Jack\"}");
    std::cout << json << std::endl;
    std::cout << "==========As Map==========" << std::endl;
    func2(json.as<std::map<std::string, std::string> >());
    func2(json.as<std::multimap<std::string, std::string> >());
    func2(json.as<std::unordered_map<std::string, std::string> >());
    func2(json.as<std::unordered_multimap<std::string, std::string> >());

    std::cout << "==========Parse==========" << std::endl;
    json.Parse("{\n\"name\": \"Jack (\\\"Bee\\\") Nimble\", \n\"format\": {\"type\":       \"rect\", \n\"width\":      1920, \n\"height\":     1080, \n\"interlace\":  false,\"frame rate\": 24\n}\n}");
    std::cout << json << std::endl;
    func2(json.as<std::map<std::string, const cppJSON &> >());
    func2(json.as<std::map<std::string, const cppJSON *> >());

    //using jw::JSON_Value;
    //typedef cppJSON JSON_Value;

    //std::cout << "==========TEST OBJECT MERGE==========" << std::endl;
    //JSON_Value temp;
    //temp.Parse("{\"name2\":\"Jack\",\"name3\":\"Jack\",\"name4\":\"Jack\",\"name5\":\"Jack\",\"name6\":\"Jack\",\"name7\":\"Jack\"}");
    //js1.merge(temp);
    //std::cout << js1 << std::endl;
    //func2(js1.as<std::map<std::string, JSON_Value> >());

    //std::cout << "==========TEST ARRAY MERGE==========" << std::endl;
    //JSON_Value js8({ "abc" });
    //js8.merge(js7);
    //std::cout << js8 << std::endl;

    //std::cout << "==========TEST OBJECT CONSTRUCT==========" << std::endl;
    //auto bbb = std::map<std::string, JSON_Value>({{ "1", JSON_Value(1) }, { "2", JSON_Value(2) }, { "3", JSON_Value(3) }});
    //JSON_Value js9(bbb);
    //std::cout << js9 << std::endl;

    //std::cout << "==========TEST OBJECT CONSTRUCT(VS2013 BUG)==========" << std::endl;
    //auto ccc = std::initializer_list<std::pair<const char*, JSON_Value> >({ { "ab", JSON_Value(1) }, { "b13", JSON_Value("b") }, { "c", JSON_Value(0.5F) } });
    //JSON_Value js10(ccc);  // 这里是VS2013的BUG
    //std::cout << js10 << std::endl;

    //std::cout << "==========TEST OBJECT CONSTRUCT==========" << std::endl;
    cppJSON js11;
    //js11.insert("123", JSON_Value("123"));
    //js11.insert("456", JSON_Value({1, 2, 3, 4, 5, 6, 7}));
    //js11.insert("789", JSON_Value(0.5F));
    //std::cout << js11 << std::endl;

    //std::cout << "==========TEST OBEJCT AS==========" << std::endl;
    //auto data11 = js11.as<std::unordered_map<std::string, JSON_Value> >();
    //auto it = data11.find("123");
    //if (it != data11.end())
    //{
    //    std::cout << it->second.as<int>() << std::endl;
    //}

    std::cout << (js11 == nullptr)  << (js11 != nullptr) << (nullptr == js11) << (nullptr != js11) << std::endl;

    std::cout << "==========Iterators==========" << std::endl;
    {
        std::vector<int> vec{0, 1, 2, 3, 4, 5, 6, 7};
        cppJSON jv = cppJSON(vec);
        std::cout << jv << std::endl;

        cppJSON jvt = jv;

        cppJSON jvtt = std::move(jvt);

        //std::map<std::string, int> bbb;
        //std::transform(jv.begin(), jv.end(), std::insert_iterator<std::map<std::string, int> >(bbb, bbb.begin()),
        //    std::bind(&std::make_pair<std::string, int>, "abc", std::bind(&cppJSON::as<int>, std::placeholders::_1)));

        std::cout << "==========Iterator==========" << std::endl;
        std::for_each(jv.begin(), jv.end(), [](const cppJSON &j) {
            std::cout << j << "  ";
        });
        std::cout << std::endl;

        {
            auto it = std::find_if(jv.rbegin(), jv.rend(), [](const cppJSON &j) { return j.as<int>() == 3; });
            std::cout << *jv.insert(it.base(), cppJSON(100)) << std::endl;
            std::cout << *jv.erase(jv.begin()) << std::endl;
            std::for_each(jv.begin(), jv.end(), [](const cppJSON &j) {
                std::cout << j << "  ";
            });
            std::cout << std::endl;
        }
        {
            auto it = std::find(vec.rbegin(), vec.rend(), 3);
            std::cout << *vec.insert(it.base(), 100) << std::endl;
            std::cout << *vec.erase(vec.begin()) << std::endl;
            //std::for_each(it.base(), vec.end(), [](int i) { std::cout << i << "  "; });
            std::for_each(vec.begin(), vec.end(), [](int i) { std::cout << i << "  "; });
            std::cout << std::endl;

            //vec.erase(vec.begin(), vec.end());
            //vec.insert()
        }

        std::cout << "==========Const Iterator==========" << std::endl;
        std::copy(jv.cbegin(), jv.cend(), std::ostream_iterator<const cppJSON &>(std::cout, ", "));
        std::cout << std::endl;

        std::cout << "==========Reverse Iterator==========" << std::endl;
        std::for_each(jv.rbegin(), jv.rend(), [](const cppJSON &j) {
            std::cout << j << "  ";
        });
        std::cout << std::endl;

        //while (!jv.empty()) jv.erase(jv.begin());
        for (auto it = jv.begin(); it != jv.end(); )
            it = jv.erase(it);
        std::cout << std::endl;
    }

    {
        cppJSON js(cppJSON::ValueType::Array);
        int arr[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        std::transform(std::begin(arr), std::end(arr), std::inserter(js, js.begin()), [](int a) {
            return cppJSON(a);
        });
        js.insert(js.begin(), { 10, 11, 12, 13, 14 });
        std::cout << js << std::endl;

        std::vector<int> vec(std::begin(arr), std::end(arr));
        vec.insert(vec.begin(), { 10, 11, 12, 13, 14 });
        std::copy(vec.begin(), vec.end(), std::ostream_iterator<int>(std::cout, " "));
        std::cout << std::endl;
    }

    {
        cppJSON js(cppJSON::ValueType::Object);
        int val[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        std::string key[] = { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j" };
        for (int i = 0; i < 10; ++i) {
            js.insert(std::make_pair(key[i], val[i]));
        }
        std::cout << js << std::endl;
        js.insert({std::make_pair(std::string("abc"), 123), std::make_pair(std::string("def"), 456)});
        js.insert(std::make_pair("123", E1_Value));
        js.insert(std::make_pair("45", E2::E2_Value));
        E1 e1 = js.GetValueByKey<E1>("abc");
        E2 e2 = js.GetValueByKey<E2>("def");
        std::cout << js << std::endl;
        

        auto it = js.find("abc");
        if (it != js.end()) {
            std::cout << it->as<double>() << std::endl;
        }
        std::cout << js.GetValueByKey<int>(std::basic_string<char, TestCharTraits, TestAllocator<char> >("def")) << std::endl;
        std::cout << js.GetValueByKeyNoThrow<double>(std::string("xyz")) << std::endl;
        std::cout << js.GetValueByKey<std::string>("def") << std::endl;
        char key1[] = "def";
        std::cout << js.GetValueByKey<std::string>(key1) << std::endl;
        const char *key2 = "def";
        std::cout << js.GetValueByKey<std::string>(key2) << std::endl;

        std::cout << "==========" << std::endl;
        for (auto it = js.begin(); it != js.end(); ++it) {
            std::cout << it->key().c_str() << ":" << *it << std::endl;
        }
        std::cout << "==========" << std::endl;
        for (auto it = js.cbegin(); it != js.cend(); ++it) {
            std::cout << it->key().c_str() << ":" << *it << std::endl;
        }
        std::cout << "==========" << std::endl;
        for (auto it = js.rbegin(); it != js.rend(); ++it) {
            std::cout << it->key().c_str() << ":" << *it << std::endl;
        }
        std::cout << "==========" << std::endl;
        for (auto it = js.crbegin(); it != js.crend(); ++it) {
            std::cout << it->key().c_str() << ":" << *it << std::endl;
        }
        std::cout << "==========" << std::endl;
    }

    //TestAllocator<int>().allocate(10);
    return 0;
}
