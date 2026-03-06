#pragma once

#if defined(_MSC_VER)
#define RENG_COMPILER_MSVC 1
#elif defined(__clang__)
#define RENG_COMPILER_CLANG 1
#elif defined(__GNUC__)
#define RENG_COMPILER_GCC 1
#else
#define RENG_COMPILER_UNKNOWN 1
#endif

#if defined(RENG_COMPILER_MSVC)
#define reng_inline __forceinline
#define reng_noinline __declspec(noinline)
#define reng_nodiscard _NODISCARD
#define reng_likely(x) (x)
#define reng_unlikely(x) (x)
#elif defined(RENG_COMPILER_CLANG) || defined(RENG_COMPILER_GCC)
#define reng_inline inline __attribute__((always_inline))
#define reng_noinline __attribute__((noinline))
#define reng_nodiscard __attribute__((warn_unused_result))
#define reng_likely(x) __builtin_expect(!!(x), 1)
#define reng_unlikely(x) __builtin_expect(!!(x), 0)
#else
#define reng_inline inline
#define reng_noinline
#define reng_nodiscard
#define reng_likely(x) (x)
#define reng_unlikely(x) (x)
#endif
