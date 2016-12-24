#ifndef _COMPILER_H_
#define _COMPILER_H_

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
#else
#define NOTHROW __attribute__((__nothrow__))
#endif
#endif

// returns nonnull
#ifndef RETURNS_NONULL
#if defined(_MSC_VER)
#define RETURNS_NONULL _Ret_notnull_
#else
#define RETURNS_NONULL __attribute__((__returns_nonnull__))
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

#endif
