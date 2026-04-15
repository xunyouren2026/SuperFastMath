/**
 * fast_math_full.h - 高性能数学函数库完整整合版
 *
 * 包含以下模块的全部原始代码（无删减）：
 *   - superfastmath  (exp, log, 双精度 + AVX2/AVX-512/NEON)
 *   - fast_rsqrt      (平方根倒数 + SSE/AVX2/AVX-512/NEON)
 *   - fast_tanh       (双曲正切 + AVX2/AVX-512/NEON)
 *   - fast_sigmoid    (Sigmoid 激活 + AVX2/AVX-512/NEON)
 *   - fast_gelu       (GELU 激活 + AVX2/AVX-512/NEON)
 *   - fast_swish      (Swish 激活 + AVX2/AVX-512/NEON)
 *   - fast_softmax    (Softmax + AVX2/AVX-512)
 *   - fast_sincos     (正弦/余弦 + AVX2/AVX-512/NEON)
 *
 * 注意：本文件由多个独立模块整合而成，保留了所有原始代码及注释。
 * 使用：定义所需配置宏后直接 #include "fast_math_full.h"
 */

#ifndef FAST_MATH_FULL_H
#define FAST_MATH_FULL_H

/* 公共标准头文件（仅包含一次） */
#include <stdint.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 忽略 GCC 严格别名警告（整个文件） */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

/* ============================================================================
 * 重复的 stdint.h 定义块（原样保留两段）
 * ============================================================================ */

/* 第一段 stdint.h 内容 */
#ifndef _STDINT_H_FIRST_BLOCK
#define _STDINT_H_FIRST_BLOCK
/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
/* ISO C9x  7.18  Integer types <stdint.h>
 * Based on ISO/IEC SC22/WG14 9899 Committee draft (SC22 N2794)
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  Contributor: Danny Smith <danny_r_smith_2001@yahoo.co.nz>
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  Date: 2000-12-02
 */

#ifndef _STDINT_H
#define _STDINT_H

#include <crtdefs.h>

#define __need_wint_t
#define __need_wchar_t
#include <stddef.h>

/* 7.18.1.1  Exact-width integer types */
typedef signed char int8_t;
typedef unsigned char   uint8_t;
typedef short  int16_t;
typedef unsigned short  uint16_t;
typedef int  int32_t;
typedef unsigned   uint32_t;
__MINGW_EXTENSION typedef long long  int64_t;
__MINGW_EXTENSION typedef unsigned long long   uint64_t;

/* 7.18.1.2  Minimum-width integer types */
typedef signed char int_least8_t;
typedef unsigned char   uint_least8_t;
typedef short  int_least16_t;
typedef unsigned short  uint_least16_t;
typedef int  int_least32_t;
typedef unsigned   uint_least32_t;
__MINGW_EXTENSION typedef long long  int_least64_t;
__MINGW_EXTENSION typedef unsigned long long   uint_least64_t;

/*  7.18.1.3  Fastest minimum-width integer types
 *  Not actually guaranteed to be fastest for all purposes
 *  Here we use the exact-width types for 8 and 16-bit ints.
 */
typedef signed char int_fast8_t;
typedef unsigned char uint_fast8_t;
typedef short  int_fast16_t;
typedef unsigned short  uint_fast16_t;
typedef int  int_fast32_t;
typedef unsigned  int  uint_fast32_t;
__MINGW_EXTENSION typedef long long  int_fast64_t;
__MINGW_EXTENSION typedef unsigned long long   uint_fast64_t;

/* 7.18.1.5  Greatest-width integer types */
__MINGW_EXTENSION typedef long long  intmax_t;
__MINGW_EXTENSION typedef unsigned long long   uintmax_t;

/* 7.18.2  Limits of specified-width integer types */
#if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS) ||	\
    defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L

/* 7.18.2.1  Limits of exact-width integer types */
#define INT8_MIN (-128)
#define INT16_MIN (-32768)
#define INT32_MIN (-2147483647 - 1)
#define INT64_MIN  (-9223372036854775807LL - 1)

#define INT8_MAX 127
#define INT16_MAX 32767
#define INT32_MAX 2147483647
#define INT64_MAX 9223372036854775807LL

#define UINT8_MAX 255
#define UINT16_MAX 65535
#define UINT32_MAX 0xffffffffU  /* 4294967295U */
#define UINT64_MAX 0xffffffffffffffffULL /* 18446744073709551615ULL */

/* 7.18.2.2  Limits of minimum-width integer types */
#define INT_LEAST8_MIN INT8_MIN
#define INT_LEAST16_MIN INT16_MIN
#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST64_MIN INT64_MIN

#define INT_LEAST8_MAX INT8_MAX
#define INT_LEAST16_MAX INT16_MAX
#define INT_LEAST32_MAX INT32_MAX
#define INT_LEAST64_MAX INT64_MAX

#define UINT_LEAST8_MAX UINT8_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#define UINT_LEAST64_MAX UINT64_MAX

/* 7.18.2.3  Limits of fastest minimum-width integer types */
#define INT_FAST8_MIN INT8_MIN
#define INT_FAST16_MIN INT16_MIN
#define INT_FAST32_MIN INT32_MIN
#define INT_FAST64_MIN INT64_MIN

#define INT_FAST8_MAX INT8_MAX
#define INT_FAST16_MAX INT16_MAX
#define INT_FAST32_MAX INT32_MAX
#define INT_FAST64_MAX INT64_MAX

#define UINT_FAST8_MAX UINT8_MAX
#define UINT_FAST16_MAX UINT16_MAX
#define UINT_FAST32_MAX UINT32_MAX
#define UINT_FAST64_MAX UINT64_MAX

/* 7.18.2.4  Limits of integer types capable of holding
    object pointers */
#ifdef _WIN64
#define INTPTR_MIN INT64_MIN
#define INTPTR_MAX INT64_MAX
#define UINTPTR_MAX UINT64_MAX
#else
#define INTPTR_MIN INT32_MIN
#define INTPTR_MAX INT32_MAX
#define UINTPTR_MAX UINT32_MAX
#endif

/* 7.18.2.5  Limits of greatest-width integer types */
#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX
#define UINTMAX_MAX UINT64_MAX

/* 7.18.3  Limits of other integer types */
#ifdef _WIN64
#define PTRDIFF_MIN INT64_MIN
#define PTRDIFF_MAX INT64_MAX
#else
#define PTRDIFF_MIN INT32_MIN
#define PTRDIFF_MAX INT32_MAX
#endif

#define SIG_ATOMIC_MIN INT32_MIN
#define SIG_ATOMIC_MAX INT32_MAX

#ifndef SIZE_MAX
#ifdef _WIN64
#define SIZE_MAX UINT64_MAX
#else
#define SIZE_MAX UINT32_MAX
#endif
#endif

#ifndef WCHAR_MIN  /* also in wchar.h */
#define WCHAR_MIN 0U
#define WCHAR_MAX 0xffffU
#endif

/*
 * wint_t is unsigned short for compatibility with MS runtime
 */
#define WINT_MIN 0U
#define WINT_MAX 0xffffU

#endif /* !defined ( __cplusplus) || defined __STDC_LIMIT_MACROS */

/* 7.18.4  Macros for integer constants */
#if !defined(__cplusplus) || defined(__STDC_CONSTANT_MACROS) ||	\
    defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L

/* 7.18.4.1  Macros for minimum-width integer constants

    Accoding to Douglas Gwyn <gwyn@arl.mil>:
	"This spec was changed in ISO/IEC 9899:1999 TC1; in ISO/IEC
	9899:1999 as initially published, the expansion was required
	to be an integer constant of precisely matching type, which
	is impossible to accomplish for the shorter types on most
	platforms, because C99 provides no standard way to designate
	an integer constant with width less than that of type int.
	TC1 changed this to require just an integer constant
	*expression* with *promoted* type."

	The trick used here is from Clive D W Feather.
*/

#define INT8_C(val) (INT_LEAST8_MAX-INT_LEAST8_MAX+(val))
#define INT16_C(val) (INT_LEAST16_MAX-INT_LEAST16_MAX+(val))
#define INT32_C(val) (INT_LEAST32_MAX-INT_LEAST32_MAX+(val))
/*  The 'trick' doesn't work in C89 for long long because, without
    suffix, (val) will be evaluated as int, not intmax_t */
#define INT64_C(val) val##LL

#define UINT8_C(val) (val)
#define UINT16_C(val) (val)
#define UINT32_C(val) (val##U)
#define UINT64_C(val) val##ULL

/* 7.18.4.2  Macros for greatest-width integer constants */
#define INTMAX_C(val) val##LL
#define UINTMAX_C(val) val##ULL

#endif  /* !defined ( __cplusplus) || defined __STDC_CONSTANT_MACROS */

#endif  /* _STDINT_H */
#endif /* _STDINT_H_FIRST_BLOCK */

/* 第二段 stdint.h 内容（原样重复） */
#ifndef _STDINT_H_SECOND_BLOCK
#define _STDINT_H_SECOND_BLOCK
/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
/* ISO C9x  7.18  Integer types <stdint.h>
 * Based on ISO/IEC SC22/WG14 9899 Committee draft (SC22 N2794)
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  Contributor: Danny Smith <danny_r_smith_2001@yahoo.co.nz>
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  Date: 2000-12-02
 */

#ifndef _STDINT_H
#define _STDINT_H

#include <crtdefs.h>

#define __need_wint_t
#define __need_wchar_t
#include <stddef.h>

/* 7.18.1.1  Exact-width integer types */
typedef signed char int8_t;
typedef unsigned char   uint8_t;
typedef short  int16_t;
typedef unsigned short  uint16_t;
typedef int  int32_t;
typedef unsigned   uint32_t;
__MINGW_EXTENSION typedef long long  int64_t;
__MINGW_EXTENSION typedef unsigned long long   uint64_t;

/* 7.18.1.2  Minimum-width integer types */
typedef signed char int_least8_t;
typedef unsigned char   uint_least8_t;
typedef short  int_least16_t;
typedef unsigned short  uint_least16_t;
typedef int  int_least32_t;
typedef unsigned   uint_least32_t;
__MINGW_EXTENSION typedef long long  int_least64_t;
__MINGW_EXTENSION typedef unsigned long long   uint_least64_t;

/*  7.18.1.3  Fastest minimum-width integer types
 *  Not actually guaranteed to be fastest for all purposes
 *  Here we use the exact-width types for 8 and 16-bit ints.
 */
typedef signed char int_fast8_t;
typedef unsigned char uint_fast8_t;
typedef short  int_fast16_t;
typedef unsigned short  uint_fast16_t;
typedef int  int_fast32_t;
typedef unsigned  int  uint_fast32_t;
__MINGW_EXTENSION typedef long long  int_fast64_t;
__MINGW_EXTENSION typedef unsigned long long   uint_fast64_t;

/* 7.18.1.5  Greatest-width integer types */
__MINGW_EXTENSION typedef long long  intmax_t;
__MINGW_EXTENSION typedef unsigned long long   uintmax_t;

/* 7.18.2  Limits of specified-width integer types */
#if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS) ||	\
    defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L

/* 7.18.2.1  Limits of exact-width integer types */
#define INT8_MIN (-128)
#define INT16_MIN (-32768)
#define INT32_MIN (-2147483647 - 1)
#define INT64_MIN  (-9223372036854775807LL - 1)

#define INT8_MAX 127
#define INT16_MAX 32767
#define INT32_MAX 2147483647
#define INT64_MAX 9223372036854775807LL

#define UINT8_MAX 255
#define UINT16_MAX 65535
#define UINT32_MAX 0xffffffffU  /* 4294967295U */
#define UINT64_MAX 0xffffffffffffffffULL /* 18446744073709551615ULL */

/* 7.18.2.2  Limits of minimum-width integer types */
#define INT_LEAST8_MIN INT8_MIN
#define INT_LEAST16_MIN INT16_MIN
#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST64_MIN INT64_MIN

#define INT_LEAST8_MAX INT8_MAX
#define INT_LEAST16_MAX INT16_MAX
#define INT_LEAST32_MAX INT32_MAX
#define INT_LEAST64_MAX INT64_MAX

#define UINT_LEAST8_MAX UINT8_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#define UINT_LEAST64_MAX UINT64_MAX

/* 7.18.2.3  Limits of fastest minimum-width integer types */
#define INT_FAST8_MIN INT8_MIN
#define INT_FAST16_MIN INT16_MIN
#define INT_FAST32_MIN INT32_MIN
#define INT_FAST64_MIN INT64_MIN

#define INT_FAST8_MAX INT8_MAX
#define INT_FAST16_MAX INT16_MAX
#define INT_FAST32_MAX INT32_MAX
#define INT_FAST64_MAX INT64_MAX

#define UINT_FAST8_MAX UINT8_MAX
#define UINT_FAST16_MAX UINT16_MAX
#define UINT_FAST32_MAX UINT32_MAX
#define UINT_FAST64_MAX UINT64_MAX

/* 7.18.2.4  Limits of integer types capable of holding
    object pointers */
#ifdef _WIN64
#define INTPTR_MIN INT64_MIN
#define INTPTR_MAX INT64_MAX
#define UINTPTR_MAX UINT64_MAX
#else
#define INTPTR_MIN INT32_MIN
#define INTPTR_MAX INT32_MAX
#define UINTPTR_MAX UINT32_MAX
#endif

/* 7.18.2.5  Limits of greatest-width integer types */
#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX
#define UINTMAX_MAX UINT64_MAX

/* 7.18.3  Limits of other integer types */
#ifdef _WIN64
#define PTRDIFF_MIN INT64_MIN
#define PTRDIFF_MAX INT64_MAX
#else
#define PTRDIFF_MIN INT32_MIN
#define PTRDIFF_MAX INT32_MAX
#endif

#define SIG_ATOMIC_MIN INT32_MIN
#define SIG_ATOMIC_MAX INT32_MAX

#ifndef SIZE_MAX
#ifdef _WIN64
#define SIZE_MAX UINT64_MAX
#else
#define SIZE_MAX UINT32_MAX
#endif
#endif

#ifndef WCHAR_MIN  /* also in wchar.h */
#define WCHAR_MIN 0U
#define WCHAR_MAX 0xffffU
#endif

/*
 * wint_t is unsigned short for compatibility with MS runtime
 */
#define WINT_MIN 0U
#define WINT_MAX 0xffffU

#endif /* !defined ( __cplusplus) || defined __STDC_LIMIT_MACROS */

/* 7.18.4  Macros for integer constants */
#if !defined(__cplusplus) || defined(__STDC_CONSTANT_MACROS) ||	\
    defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L

/* 7.18.4.1  Macros for minimum-width integer constants

    Accoding to Douglas Gwyn <gwyn@arl.mil>:
	"This spec was changed in ISO/IEC 9899:1999 TC1; in ISO/IEC
	9899:1999 as initially published, the expansion was required
	to be an integer constant of precisely matching type, which
	is impossible to accomplish for the shorter types on most
	platforms, because C99 provides no standard way to designate
	an integer constant with width less than that of type int.
	TC1 changed this to require just an integer constant
	*expression* with *promoted* type."

	The trick used here is from Clive D W Feather.
*/

#define INT8_C(val) (INT_LEAST8_MAX-INT_LEAST8_MAX+(val))
#define INT16_C(val) (INT_LEAST16_MAX-INT_LEAST16_MAX+(val))
#define INT32_C(val) (INT_LEAST32_MAX-INT_LEAST32_MAX+(val))
/*  The 'trick' doesn't work in C89 for long long because, without
    suffix, (val) will be evaluated as int, not intmax_t */
#define INT64_C(val) val##LL

#define UINT8_C(val) (val)
#define UINT16_C(val) (val)
#define UINT32_C(val) (val##U)
#define UINT64_C(val) val##ULL

/* 7.18.4.2  Macros for greatest-width integer constants */
#define INTMAX_C(val) val##LL
#define UINTMAX_C(val) val##ULL

#endif  /* !defined ( __cplusplus) || defined __STDC_CONSTANT_MACROS */

#endif  /* _STDINT_H */
#endif /* _STDINT_H_SECOND_BLOCK */

/* ============================================================================
 * 为避免模块内部 #include "xxx.h" 重复包含，提前定义相关头文件保护宏
 * ============================================================================ */
#define FAST_EXPF_H
#define FAST_SIGMOIDF_H
#define FAST_TANHF_H

/* ============================================================================
 * 模块 1：SUPERFASTMATH (指数/对数)
 * ============================================================================ */
#ifndef SUPERFASTMATH_H
#define SUPERFASTMATH_H

/* ========== 平台检测与降级 ========== */
#if defined(SUPERFASTMATH_FALLBACK_STDLIB)
    #define SUPERFASTMATH_USE_STDLIB 1
#elif !defined(__STDC_IEC_559__) && !defined(__IEEE_BIG_ENDIAN) && !defined(__IEEE_LITTLE_ENDIAN)
    #define SUPERFASTMATH_USE_STDLIB 1
#else
    #define SUPERFASTMATH_USE_STDLIB 0
#endif

#if SUPERFASTMATH_USE_STDLIB
    #define fast_expf     expf
    #define fast_lnf      logf
    #define fast_log2f    log2f
    #define fast_log10f   log10f
    #define fast_exp      exp
    #define fast_ln       log
    #define fast_log2     log2
    #define fast_log10    log10
    #ifdef __AVX512F__
        #include <immintrin.h>
        #define fast_expf_avx512  _mm512_exp_ps
        #define fast_lnf_avx512   _mm512_log_ps
    #endif
#else   /* 自定义实现 */

/* ========== 标准头文件 ========== */
#ifdef SUPERFASTMATH_STRICT
    #include <errno.h>
    #include <fenv.h>
#endif

/* ========== 可移植性宏 ========== */
#ifdef _MSC_VER
    #define LIKELY(x)   (x)
    #define UNLIKELY(x) (x)
    #ifdef SUPERFASTMATH_NO_FORCE_INLINE
        #define INLINE static inline
    #else
        #define INLINE __forceinline
    #endif
#else
    #define LIKELY(x)   __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
    #ifdef SUPERFASTMATH_NO_FORCE_INLINE
        #define INLINE static inline
    #else
        #define INLINE static inline __attribute__((always_inline))
    #endif
#endif

/* ========== 安全模式配置 ========== */
#ifdef SUPERFASTMATH_STRICT
    #define MATH_ERRNO_DOMAIN()   do { errno = EDOM; } while(0)
    #define MATH_ERRNO_RANGE()    do { errno = ERANGE; } while(0)
    #define MATH_RAISE_INVALID()  feraiseexcept(FE_INVALID)
    #define MATH_RAISE_DIVBYZERO() feraiseexcept(FE_DIVBYZERO)
    #define MATH_RAISE_OVERFLOW() feraiseexcept(FE_OVERFLOW | FE_INEXACT)
    #define MATH_RAISE_UNDERFLOW() feraiseexcept(FE_UNDERFLOW | FE_INEXACT)
#else
    #define MATH_ERRNO_DOMAIN()   ((void)0)
    #define MATH_ERRNO_RANGE()    ((void)0)
    #define MATH_RAISE_INVALID()  ((void)0)
    #define MATH_RAISE_DIVBYZERO() ((void)0)
    #define MATH_RAISE_OVERFLOW() ((void)0)
    #define MATH_RAISE_UNDERFLOW() ((void)0)
#endif

/* ========== 运行时 CPU 特性检测 ========== */
#if !defined(SUPERFASTMATH_NO_RUNTIME_DISPATCH) && defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    #include <cpuid.h>
    #define SUPERFASTMATH_HAVE_AVX512 (__builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512vl"))
    #define SUPERFASTMATH_HAVE_AVX2   __builtin_cpu_supports("avx2")
    #define SUPERFASTMATH_HAVE_FMA    __builtin_cpu_supports("fma")
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    #define SUPERFASTMATH_HAVE_NEON   1
#else
    #define SUPERFASTMATH_HAVE_AVX512 0
    #define SUPERFASTMATH_HAVE_AVX2   0
    #define SUPERFASTMATH_HAVE_NEON   0
    #define SUPERFASTMATH_HAVE_FMA    0
#endif

/* ========== 精度等级 ========== */
#ifdef SUPERFASTMATH_RELAXED
    #define POLY_EXPF_ORDER 3      /* 3阶泰勒，误差 ~2e-5 */
    #define POLY_LOGF_ORDER 5      /* 5阶多项式，误差 ~2e-6 */
#else
    #define POLY_EXPF_ORDER 4      /* 4阶泰勒 */
    #define POLY_LOGF_ORDER 7      /* 7阶极小极大 */
#endif

/* ========== 单精度多项式 ========== */
INLINE float poly7_log2f(float t) {
    const float c7 = -0.02812419f;
    const float c6 =  0.05306968f;
    const float c5 = -0.10068947f;
    const float c4 =  0.16696936f;
    const float c3 = -0.24996818f;
    const float c2 =  0.33332115f;
    const float c1 = -0.49999994f;
    const float c0 =  1.00000000f;
    float t2 = t * t;
    float t4 = t2 * t2;
    float p1 = (c7 * t + c6) * t2 + (c5 * t + c4);
    float p2 = (c3 * t + c2) * t2 + (c1 * t + c0);
    return (p1 * t4 + p2) * t;
}

INLINE float poly5_log2f(float t) {
    const float c5 = -0.152319f;
    const float c4 =  0.271719f;
    const float c3 = -0.499809f;
    const float c2 =  1.000000f;
    const float c1 =  0.000000f;
    float t2 = t * t;
    float t3 = t2 * t;
    return (((c5 * t + c4) * t + c3) * t + c2) * t;
}

#if POLY_LOGF_ORDER == 7
    #define poly_log2f_impl poly7_log2f
#else
    #define poly_log2f_impl poly5_log2f
#endif

/* ========== 单精度 expf（含正确舍入修正） ========== */
INLINE float fast_expf(float x) {
    const float ln2     = 0.6931471805599453f;
    const float inv_ln2 = 1.4426950408889634f;
    const float rnd     = 12582912.0f;

    float kf = x * inv_ln2 + rnd;
    int k = (int)kf - (int)rnd;

#ifdef SUPERFASTMATH_BRANCHLESS
    uint32_t mask_overflow  = (uint32_t)((k - 128) >> 31);
    uint32_t mask_underflow = (uint32_t)((-127 - k) >> 31);
    uint32_t inf_bits = 0x7F800000U;
    uint32_t normal_bits = (uint32_t)(k + 127) << 23;
    union { float f; uint32_t i; } u;
    u.i = (mask_overflow & inf_bits) | ((~mask_overflow & ~mask_underflow) & normal_bits);
    float pow2 = u.f;
    uint32_t zero_mask = mask_underflow;
    u.i &= ~zero_mask;
    pow2 = u.f;
#else
    if (UNLIKELY(k > 127)) {
        MATH_RAISE_OVERFLOW(); MATH_ERRNO_RANGE();
        union { float f; uint32_t i; } u; u.i = 0x7F800000U; return u.f;
    }
    if (UNLIKELY(k < -126)) {
        MATH_RAISE_UNDERFLOW(); MATH_ERRNO_RANGE();
        return 0.0f;
    }
    union { float f; uint32_t i; } u;
    u.i = (uint32_t)(k + 127) << 23;
    float pow2 = u.f;
#endif

    float r = x - (float)k * ln2;
    float r2 = r * r;
    float r3 = r2 * r;
    float r4 = r3 * r;
#if POLY_EXPF_ORDER == 4
    float exp_r = 1.0f + r + r2 * 0.5f + r3 * 0.16666667f + r4 * 0.041666667f;
#else
    float exp_r = 1.0f + r + r2 * 0.5f + r3 * 0.16666667f;
#endif

    float result = pow2 * exp_r;

#ifdef SUPERFASTMATH_CORRECT_ROUND
    /* Halley 迭代修正 */
    float y = result;
    float ln_y = fast_lnf(y);
    float residual = x - ln_y;
    float rc2 = residual * residual;
    float rc3 = rc2 * residual;
    float correction = 1.0f + residual + rc2 * 0.5f + rc3 * 0.16666667f;
    result = y * correction;
#endif

    return result;
}

/* ========== 单精度 lnf ========== */
INLINE float fast_lnf(float x) {
    union { float f; uint32_t i; } u = {x};
    uint32_t ix = u.i;
    int32_t exp = (ix >> 23) & 0xFF;

    if (LIKELY(exp - 1U < 254 - 1U)) {
        exp -= 127;
        ix = (ix & 0x7FFFFF) | 0x3F800000;
        u.i = ix;
        float m = u.f;
        float log2_m = poly_log2f_impl(m - 1.0f);
        const float ln2 = 0.6931471805599453f;
        return (log2_m + (float)exp) * ln2;
    }

    if (UNLIKELY(exp == 0xFF)) {
        if (ix == 0x7F800000) return INFINITY;
        if (ix == 0xFF800000) {
            MATH_RAISE_INVALID(); MATH_ERRNO_DOMAIN();
            union { float f; uint32_t i; } nan; nan.i = 0x7FC00000U; return nan.f;
        }
        return x + x;
    }
    if (UNLIKELY(ix == 0)) {
        MATH_RAISE_DIVBYZERO(); MATH_ERRNO_RANGE(); return -INFINITY;
    }
    if (UNLIKELY(ix & 0x80000000)) {
        MATH_RAISE_INVALID(); MATH_ERRNO_DOMAIN();
        union { float f; uint32_t i; } nan; nan.i = 0x7FC00000U; return nan.f;
    }

    uint32_t mantissa = ix & 0x7FFFFF;
    u.i = (127 << 23) | mantissa;
    float log2_m = poly_log2f_impl(u.f - 1.0f);
    const float ln2 = 0.6931471805599453f;
    return (log2_m - 126.0f) * ln2;
}

INLINE float fast_log2f(float x) { return fast_lnf(x) * 1.4426950408889634f; }
INLINE float fast_log10f(float x) { return fast_lnf(x) * 0.4342944819032518f; }

/* ========== 双精度版本（含正确舍入修正） ========== */
INLINE double poly8_log2(double t) {
    const double c9 = -0.01574706;
    const double c8 =  0.02733031;
    const double c7 = -0.05338396;
    const double c6 =  0.10014339;
    const double c5 = -0.16673829;
    const double c4 =  0.24999999;
    const double c3 = -0.33333333;
    const double c2 =  0.50000000;
    const double c1 = -1.00000000;
    const double c0 =  0.00000000;
    double t2 = t * t;
    double t4 = t2 * t2;
    double p1 = (c9 * t + c8) * t2 + (c7 * t + c6);
    double p2 = (c5 * t + c4) * t2 + (c3 * t + c2);
    return (p1 * t4 + p2) * t2 + t;
}

INLINE double fast_exp(double x) {
    const double ln2     = 0.6931471805599453;
    const double inv_ln2 = 1.4426950408889634;
    const double rnd     = 6755399441055744.0;
    double kf = x * inv_ln2 + rnd;
    int64_t k = (int64_t)kf - (int64_t)rnd;

    if (UNLIKELY(k > 1023)) {
        MATH_RAISE_OVERFLOW(); MATH_ERRNO_RANGE();
        union { double d; uint64_t i; } u; u.i = 0x7FF0000000000000ULL; return u.d;
    }
    if (UNLIKELY(k < -1022)) {
        MATH_RAISE_UNDERFLOW(); MATH_ERRNO_RANGE(); return 0.0;
    }
    union { double d; uint64_t i; } u;
    u.i = (uint64_t)(k + 1023) << 52;
    double pow2 = u.d;

    double r = x - (double)k * ln2;
    double r2 = r * r;
    double r3 = r2 * r;
    double r4 = r3 * r;
    double r5 = r4 * r;
    double exp_r = 1.0 + r + r2/2.0 + r3/6.0 + r4/24.0 + r5/120.0;
    double result = pow2 * exp_r;

#ifdef SUPERFASTMATH_CORRECT_ROUND
    double y = result;
    double ln_y = fast_ln(y);
    double residual = x - ln_y;
    double rc2 = residual * residual;
    double rc3 = rc2 * residual;
    double correction = 1.0 + residual + rc2/2.0 + rc3/6.0;
    result = y * correction;
#endif

    return result;
}

INLINE double fast_ln(double x) {
    union { double d; uint64_t i; } u = {x};
    uint64_t ix = u.i;
    int64_t exp = (ix >> 52) & 0x7FF;

    if (LIKELY(exp - 1ULL < 0x7FF - 1ULL)) {
        exp -= 1023;
        ix = (ix & 0xFFFFFFFFFFFFFULL) | 0x3FF0000000000000ULL;
        u.i = ix;
        double m = u.d;
        double log2_m = poly8_log2(m - 1.0);
        const double ln2 = 0.6931471805599453;
        return (log2_m + (double)exp) * ln2;
    }

    if (UNLIKELY(exp == 0x7FF)) {
        if (ix == 0x7FF0000000000000ULL) return INFINITY;
        if (ix == 0xFFF0000000000000ULL) {
            MATH_RAISE_INVALID(); MATH_ERRNO_DOMAIN();
            union { double d; uint64_t i; } nan; nan.i = 0x7FF8000000000000ULL; return nan.d;
        }
        return x + x;
    }
    if (UNLIKELY(ix == 0)) {
        MATH_RAISE_DIVBYZERO(); MATH_ERRNO_RANGE(); return -INFINITY;
    }
    if (UNLIKELY(ix & 0x8000000000000000ULL)) {
        MATH_RAISE_INVALID(); MATH_ERRNO_DOMAIN();
        union { double d; uint64_t i; } nan; nan.i = 0x7FF8000000000000ULL; return nan.d;
    }

    uint64_t mantissa = ix & 0xFFFFFFFFFFFFFULL;
    u.i = (1023ULL << 52) | mantissa;
    double log2_m = poly8_log2(u.d - 1.0);
    const double ln2 = 0.6931471805599453;
    return (log2_m - 1022.0) * ln2;
}

INLINE double fast_log2(double x) { return fast_ln(x) * 1.4426950408889634; }
INLINE double fast_log10(double x) { return fast_ln(x) * 0.4342944819032518; }

/* ========== 向量化内核 ========== */

/* --- AVX2 单精度 --- */
#if defined(__AVX2__) && defined(SUPERFASTMATH_HAVE_AVX2)
#include <immintrin.h>

INLINE __m256 fast_expf_avx2(__m256 x) {
    const __m256 ln2     = _mm256_set1_ps(0.6931471805599453f);
    const __m256 inv_ln2 = _mm256_set1_ps(1.4426950408889634f);
    const __m256 rnd     = _mm256_set1_ps(12582912.0f);
    __m256 kf = _mm256_fmadd_ps(x, inv_ln2, rnd);
    __m256i k = _mm256_sub_epi32(_mm256_cvttps_epi32(kf), _mm256_set1_epi32(12582912));
    __m256 r = _mm256_fnmadd_ps(_mm256_cvtepi32_ps(k), ln2, x);
    __m256 r2 = _mm256_mul_ps(r, r);
    __m256 r3 = _mm256_mul_ps(r2, r);
    __m256 r4 = _mm256_mul_ps(r3, r);
    __m256 exp_r = _mm256_fmadd_ps(r4, _mm256_set1_ps(1.0f/24.0f),
                   _mm256_fmadd_ps(r3, _mm256_set1_ps(1.0f/6.0f),
                   _mm256_fmadd_ps(r2, _mm256_set1_ps(0.5f),
                   _mm256_add_ps(r, _mm256_set1_ps(1.0f)))));
    __m256i exp_biased = _mm256_add_epi32(k, _mm256_set1_epi32(127));
    exp_biased = _mm256_slli_epi32(exp_biased, 23);
    __m256 pow2 = _mm256_castsi256_ps(exp_biased);
    return _mm256_mul_ps(pow2, exp_r);
}

INLINE __m256 fast_lnf_avx2(__m256 x) {
    __m256i xi = _mm256_castps_si256(x);
    __m256i exp = _mm256_srli_epi32(xi, 23);
    exp = _mm256_sub_epi32(exp, _mm256_set1_epi32(127));
    xi = _mm256_and_si256(xi, _mm256_set1_epi32(0x7FFFFF));
    xi = _mm256_or_si256(xi, _mm256_set1_epi32(0x3F800000));
    __m256 m = _mm256_castsi256_ps(xi);
    __m256 t = _mm256_sub_ps(m, _mm256_set1_ps(1.0f));
    const __m256 c7 = _mm256_set1_ps(-0.02812419f);
    const __m256 c6 = _mm256_set1_ps( 0.05306968f);
    const __m256 c5 = _mm256_set1_ps(-0.10068947f);
    const __m256 c4 = _mm256_set1_ps( 0.16696936f);
    const __m256 c3 = _mm256_set1_ps(-0.24996818f);
    const __m256 c2 = _mm256_set1_ps( 0.33332115f);
    const __m256 c1 = _mm256_set1_ps(-0.49999994f);
    const __m256 c0 = _mm256_set1_ps( 1.00000000f);
    __m256 t2 = _mm256_mul_ps(t, t);
    __m256 t4 = _mm256_mul_ps(t2, t2);
    __m256 p1 = _mm256_fmadd_ps(c7, t, c6);
    p1 = _mm256_fmadd_ps(p1, t2, _mm256_fmadd_ps(c5, t, c4));
    __m256 p2 = _mm256_fmadd_ps(c3, t, c2);
    p2 = _mm256_fmadd_ps(p2, t2, _mm256_fmadd_ps(c1, t, c0));
    __m256 log2_m = _mm256_mul_ps(_mm256_fmadd_ps(p1, t4, p2), t);
    __m256 expf = _mm256_cvtepi32_ps(exp);
    const __m256 ln2 = _mm256_set1_ps(0.6931471805599453f);
    return _mm256_mul_ps(_mm256_add_ps(log2_m, expf), ln2);
}
#endif /* __AVX2__ */

/* --- ARM NEON 单精度 --- */
#if defined(__ARM_NEON) && defined(SUPERFASTMATH_HAVE_NEON)
#include <arm_neon.h>

INLINE float32x4_t fast_expf_neon(float32x4_t x) {
    const float32x4_t ln2     = vdupq_n_f32(0.6931471805599453f);
    const float32x4_t inv_ln2 = vdupq_n_f32(1.4426950408889634f);
    const float32x4_t rnd     = vdupq_n_f32(12582912.0f);
    float32x4_t kf = vmlaq_f32(rnd, x, inv_ln2);
    int32x4_t k = vsubq_s32(vcvtq_s32_f32(kf), vdupq_n_s32(12582912));
    float32x4_t r = vmlsq_f32(x, vcvtq_f32_s32(k), ln2);
    float32x4_t r2 = vmulq_f32(r, r);
    float32x4_t r3 = vmulq_f32(r2, r);
    float32x4_t r4 = vmulq_f32(r3, r);
    float32x4_t exp_r = vmlaq_f32(vdupq_n_f32(1.0f), r,
                        vmlaq_f32(vmulq_f32(r2, vdupq_n_f32(0.5f)),
                                  vmulq_f32(r3, vdupq_n_f32(1.0f/6.0f)),
                                  vmulq_f32(r4, vdupq_n_f32(1.0f/24.0f))));
    int32x4_t exp_biased = vaddq_s32(k, vdupq_n_s32(127));
    exp_biased = vshlq_n_s32(exp_biased, 23);
    float32x4_t pow2 = vreinterpretq_f32_s32(exp_biased);
    return vmulq_f32(pow2, exp_r);
}

INLINE float32x4_t fast_lnf_neon(float32x4_t x) {
    int32x4_t xi = vreinterpretq_s32_f32(x);
    int32x4_t exp = vshrq_n_s32(xi, 23);
    exp = vsubq_s32(exp, vdupq_n_s32(127));
    xi = vandq_s32(xi, vdupq_n_s32(0x7FFFFF));
    xi = vorrq_s32(xi, vdupq_n_s32(0x3F800000));
    float32x4_t m = vreinterpretq_f32_s32(xi);
    float32x4_t t = vsubq_f32(m, vdupq_n_f32(1.0f));
    const float32x4_t c7 = vdupq_n_f32(-0.02812419f);
    const float32x4_t c6 = vdupq_n_f32( 0.05306968f);
    const float32x4_t c5 = vdupq_n_f32(-0.10068947f);
    const float32x4_t c4 = vdupq_n_f32( 0.16696936f);
    const float32x4_t c3 = vdupq_n_f32(-0.24996818f);
    const float32x4_t c2 = vdupq_n_f32( 0.33332115f);
    const float32x4_t c1 = vdupq_n_f32(-0.49999994f);
    const float32x4_t c0 = vdupq_n_f32( 1.00000000f);
    float32x4_t t2 = vmulq_f32(t, t);
    float32x4_t t4 = vmulq_f32(t2, t2);
    float32x4_t p1 = vmlaq_f32(c6, c7, t);
    p1 = vmlaq_f32(vmlaq_f32(c4, c5, t), p1, t2);
    float32x4_t p2 = vmlaq_f32(c2, c3, t);
    p2 = vmlaq_f32(vmlaq_f32(c0, c1, t), p2, t2);
    float32x4_t log2_m = vmulq_f32(vmlaq_f32(p2, p1, t4), t);
    float32x4_t expf = vcvtq_f32_s32(exp);
    const float32x4_t ln2 = vdupq_n_f32(0.6931471805599453f);
    return vmulq_f32(vaddq_f32(log2_m, expf), ln2);
}
#endif /* __ARM_NEON */

/* --- AVX-512 单精度 --- */
#if defined(__AVX512F__) && defined(SUPERFASTMATH_HAVE_AVX512)
#include <immintrin.h>

INLINE __m512 fast_expf_avx512(__m512 x) {
    const __m512 ln2     = _mm512_set1_ps(0.6931471805599453f);
    const __m512 inv_ln2 = _mm512_set1_ps(1.4426950408889634f);
    const __m512 rnd     = _mm512_set1_ps(12582912.0f);
    __m512 kf = _mm512_fmadd_ps(x, inv_ln2, rnd);
    __m512i k = _mm512_sub_epi32(_mm512_cvttps_epi32(kf), _mm512_set1_epi32(12582912));
    __m512 r = _mm512_fnmadd_ps(_mm512_cvtepi32_ps(k), ln2, x);
    __m512 r2 = _mm512_mul_ps(r, r);
    __m512 r3 = _mm512_mul_ps(r2, r);
    __m512 r4 = _mm512_mul_ps(r3, r);
    __m512 exp_r = _mm512_fmadd_ps(r4, _mm512_set1_ps(1.0f/24.0f),
                   _mm512_fmadd_ps(r3, _mm512_set1_ps(1.0f/6.0f),
                   _mm512_fmadd_ps(r2, _mm512_set1_ps(0.5f),
                   _mm512_add_ps(r, _mm512_set1_ps(1.0f)))));
    __m512i exp_biased = _mm512_add_epi32(k, _mm512_set1_epi32(127));
    exp_biased = _mm512_slli_epi32(exp_biased, 23);
    __m512 pow2 = _mm512_castsi512_ps(exp_biased);
    return _mm512_mul_ps(pow2, exp_r);
}

INLINE __m512 fast_lnf_avx512(__m512 x) {
    __m512i xi = _mm512_castps_si512(x);
    __m512i exp = _mm512_srli_epi32(xi, 23);
    exp = _mm512_sub_epi32(exp, _mm512_set1_epi32(127));
    xi = _mm512_and_si512(xi, _mm512_set1_epi32(0x7FFFFF));
    xi = _mm512_or_si512(xi, _mm512_set1_epi32(0x3F800000));
    __m512 m = _mm512_castsi512_ps(xi);
    __m512 t = _mm512_sub_ps(m, _mm512_set1_ps(1.0f));
    const __m512 c7 = _mm512_set1_ps(-0.02812419f);
    const __m512 c6 = _mm512_set1_ps( 0.05306968f);
    const __m512 c5 = _mm512_set1_ps(-0.10068947f);
    const __m512 c4 = _mm512_set1_ps( 0.16696936f);
    const __m512 c3 = _mm512_set1_ps(-0.24996818f);
    const __m512 c2 = _mm512_set1_ps( 0.33332115f);
    const __m512 c1 = _mm512_set1_ps(-0.49999994f);
    const __m512 c0 = _mm512_set1_ps( 1.00000000f);
    __m512 t2 = _mm512_mul_ps(t, t);
    __m512 t4 = _mm512_mul_ps(t2, t2);
    __m512 p1 = _mm512_fmadd_ps(c7, t, c6);
    p1 = _mm512_fmadd_ps(p1, t2, _mm512_fmadd_ps(c5, t, c4));
    __m512 p2 = _mm512_fmadd_ps(c3, t, c2);
    p2 = _mm512_fmadd_ps(p2, t2, _mm512_fmadd_ps(c1, t, c0));
    __m512 log2_m = _mm512_mul_ps(_mm512_fmadd_ps(p1, t4, p2), t);
    __m512 expf = _mm512_cvtepi32_ps(exp);
    const __m512 ln2 = _mm512_set1_ps(0.6931471805599453f);
    return _mm512_mul_ps(_mm512_add_ps(log2_m, expf), ln2);
}
#endif /* __AVX512F__ */

/* --- 双精度 AVX-512 向量化（可选模块） --- */
#if defined(SUPERFASTMATH_DOUBLE_VECTOR) && defined(__AVX512F__) && defined(SUPERFASTMATH_HAVE_AVX512)
INLINE __m512d fast_exp_avx512d(__m512d x) {
    const __m512d ln2     = _mm512_set1_pd(0.6931471805599453);
    const __m512d inv_ln2 = _mm512_set1_pd(1.4426950408889634);
    const __m512d rnd     = _mm512_set1_pd(6755399441055744.0);
    __m512d kf = _mm512_fmadd_pd(x, inv_ln2, rnd);
    __m512i k = _mm512_sub_epi64(_mm512_cvttpd_epi64(kf), _mm512_set1_epi64(6755399441055744));
    __m512d r = _mm512_fnmadd_pd(_mm512_cvtepi64_pd(k), ln2, x);
    __m512d r2 = _mm512_mul_pd(r, r);
    __m512d r3 = _mm512_mul_pd(r2, r);
    __m512d r4 = _mm512_mul_pd(r3, r);
    __m512d r5 = _mm512_mul_pd(r4, r);
    __m512d exp_r = _mm512_fmadd_pd(r5, _mm512_set1_pd(1.0/120.0),
                    _mm512_fmadd_pd(r4, _mm512_set1_pd(1.0/24.0),
                    _mm512_fmadd_pd(r3, _mm512_set1_pd(1.0/6.0),
                    _mm512_fmadd_pd(r2, _mm512_set1_pd(0.5),
                    _mm512_add_pd(r, _mm512_set1_pd(1.0))))));
    __m512i exp_biased = _mm512_add_epi64(k, _mm512_set1_epi64(1023));
    exp_biased = _mm512_slli_epi64(exp_biased, 52);
    __m512d pow2 = _mm512_castsi512_pd(exp_biased);
    return _mm512_mul_pd(pow2, exp_r);
}

INLINE __m512d fast_ln_avx512d(__m512d x) {
    __m512i xi = _mm512_castpd_si512(x);
    __m512i exp = _mm512_srli_epi64(xi, 52);
    exp = _mm512_sub_epi64(exp, _mm512_set1_epi64(1023));
    xi = _mm512_and_si512(xi, _mm512_set1_epi64(0xFFFFFFFFFFFFFULL));
    xi = _mm512_or_si512(xi, _mm512_set1_epi64(0x3FF0000000000000ULL));
    __m512d m = _mm512_castsi512_pd(xi);
    __m512d t = _mm512_sub_pd(m, _mm512_set1_pd(1.0));
    /* 使用与标量相同的 9 阶多项式 */
    const __m512d c9 = _mm512_set1_pd(-0.01574706);
    const __m512d c8 = _mm512_set1_pd( 0.02733031);
    const __m512d c7 = _mm512_set1_pd(-0.05338396);
    const __m512d c6 = _mm512_set1_pd( 0.10014339);
    const __m512d c5 = _mm512_set1_pd(-0.16673829);
    const __m512d c4 = _mm512_set1_pd( 0.24999999);
    const __m512d c3 = _mm512_set1_pd(-0.33333333);
    const __m512d c2 = _mm512_set1_pd( 0.50000000);
    const __m512d c1 = _mm512_set1_pd(-1.00000000);
    __m512d t2 = _mm512_mul_pd(t, t);
    __m512d t4 = _mm512_mul_pd(t2, t2);
    __m512d p1 = _mm512_fmadd_pd(c9, t, c8);
    p1 = _mm512_fmadd_pd(p1, t2, _mm512_fmadd_pd(c7, t, c6));
    __m512d p2 = _mm512_fmadd_pd(c5, t, c4);
    p2 = _mm512_fmadd_pd(p2, t2, _mm512_fmadd_pd(c3, t, c2));
    __m512d log2_m = _mm512_add_pd(_mm512_mul_pd(_mm512_fmadd_pd(p1, t4, p2), t2), t);
    __m512d expf = _mm512_cvtepi64_pd(exp);
    const __m512d ln2 = _mm512_set1_pd(0.6931471805599453);
    return _mm512_mul_pd(_mm512_add_pd(log2_m, expf), ln2);
}
#endif /* DOUBLE_VECTOR && AVX512F */

#endif /* SUPERFASTMATH_USE_STDLIB */
#endif /* SUPERFASTMATH_H */

/* 提前声明向量函数，供后续模块使用（如果未定义则定义） */
#ifndef FAST_EXPF_AVX2_DECLARED
#define FAST_EXPF_AVX2_DECLARED
#endif
#ifndef FAST_EXPF_AVX512_DECLARED
#define FAST_EXPF_AVX512_DECLARED
#endif
#ifndef FAST_EXPF_NEON_DECLARED
#define FAST_EXPF_NEON_DECLARED
#endif

/* ============================================================================
 * 模块 2：FAST_RSQRT (平方根倒数)
 * ============================================================================ */
#ifndef FAST_RSQRT_H
#define FAST_RSQRT_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 鍙Щ妞嶆€у畯 ========== */
#ifdef _MSC_VER
    #define FAST_RSQRT_INLINE static __forceinline
    #define FAST_RSQRT_LIKELY(x)   (x)
    #define FAST_RSQRT_UNLIKELY(x) (x)
#else
    #define FAST_RSQRT_INLINE static inline __attribute__((always_inline))
    #define FAST_RSQRT_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define FAST_RSQRT_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

/* ========== 绮惧害绛夌骇閰嶇疆 ========== */
#ifdef FAST_RSQRT_ACCURATE
    #define RSQRT_NEWTON_STEPS 2
#else
    #define RSQRT_NEWTON_STEPS 1
#endif

/* ========== 杩愯鏃?CPU 鐗规€ф娴?========== */
#if !defined(FAST_RSQRT_NO_RUNTIME) && defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    #include <cpuid.h>
    #define FAST_RSQRT_HAVE_AVX512 (__builtin_cpu_supports("avx512f"))
    #define FAST_RSQRT_HAVE_AVX2   (__builtin_cpu_supports("avx2"))
    #define FAST_RSQRT_HAVE_SSE    (__builtin_cpu_supports("sse"))
#elif defined(__AVX512F__)
    #define FAST_RSQRT_HAVE_AVX512 1
#elif defined(__AVX2__)
    #define FAST_RSQRT_HAVE_AVX2 1
#elif defined(__SSE__)
    #define FAST_RSQRT_HAVE_SSE 1
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    #define FAST_RSQRT_HAVE_NEON 1
#else
    #define FAST_RSQRT_HAVE_AVX512 0
    #define FAST_RSQRT_HAVE_AVX2   0
    #define FAST_RSQRT_HAVE_SSE    0
    #define FAST_RSQRT_HAVE_NEON   0
#endif

/* ========== 鏍囬噺绾蒋浠跺疄鐜?(璺ㄥ钩鍙帮紝鏃犳寚浠や緷璧? ========== */
FAST_RSQRT_INLINE float fast_rsqrtf_software(float x) {
    union { float f; uint32_t i; } u = {x};
    uint32_t ix = u.i;

    // 蹇€熻矾寰勶細姝ｅ父姝ｆ暟
    if (FAST_RSQRT_LIKELY(ix > 0 && ix < 0x7F800000U)) {
        // 鏀硅繘榄旀硶甯告暟锛?x5f37642f (姣?Quake 鐨?0x5f3759df 璇樊闄嶄綆绾?30%)
        u.i = 0x5f37642f - (ix >> 1);
        float y = u.f;

        // 鐗涢】杩唬锛歽 = y * (1.5 - 0.5 * x * y * y)
        float x2 = x * 0.5f;
        y = y * (1.5f - x2 * y * y);
#if RSQRT_NEWTON_STEPS >= 2
        y = y * (1.5f - x2 * y * y);  // 绗簩娆¤凯浠ｏ紝绮惧害 < 1e-6
#endif
        return y;
    }

    // 鐗规畩鍊煎鐞?
    if (FAST_RSQRT_UNLIKELY(ix == 0)) {
        u.i = 0x7F800000U; return u.f;      // 0 -> +Inf
    }
    if (FAST_RSQRT_UNLIKELY(ix >= 0x7F800000U)) {
        if (ix == 0x7F800000U) return 0.0f;  // +Inf -> 0
        return x + x;                        // NaN / -Inf -> NaN
    }
    if (FAST_RSQRT_UNLIKELY(ix & 0x80000000U)) {
        u.i = 0x7FC00000U; return u.f;       // 璐熸暟 -> NaN
    }

    // 闈炶鏍煎寲鏁帮細鏀惧ぇ鍚庨€掑綊
    u.f = x * 0x1.0p64f;
    return fast_rsqrtf_software(u.f) * 0x1.0p32f;
}

/* ========== x86 SSE 纭欢鍔犻€熺増 ========== */
#if defined(__SSE__) || FAST_RSQRT_HAVE_SSE
#include <xmmintrin.h>

FAST_RSQRT_INLINE float fast_rsqrtf_sse(float x) {
    __m128 v = _mm_set_ss(x);
    v = _mm_rsqrt_ss(v);                    // 纭欢鍒濆€硷紝寤惰繜 4-5 cycles
    float y = _mm_cvtss_f32(v);

    float x2 = x * 0.5f;
    y = y * (1.5f - x2 * y * y);
#if RSQRT_NEWTON_STEPS >= 2
    y = y * (1.5f - x2 * y * y);
#endif
    return y;
}
#endif

/* ========== AVX2 鍚戦噺鍖栫増 (8涓猣loat骞惰) ========== */
#if defined(__AVX2__) || FAST_RSQRT_HAVE_AVX2
#include <immintrin.h>

FAST_RSQRT_INLINE __m256 fast_rsqrtf_avx2(__m256 x) {
    __m256 y = _mm256_rsqrt_ps(x);          // 纭欢鍒濆€?
    __m256 half = _mm256_set1_ps(0.5f);
    __m256 one_point_five = _mm256_set1_ps(1.5f);
    __m256 x_half = _mm256_mul_ps(x, half);
    __m256 y_sq = _mm256_mul_ps(y, y);
    y = _mm256_mul_ps(y, _mm256_fnmadd_ps(x_half, y_sq, one_point_five));
#if RSQRT_NEWTON_STEPS >= 2
    y_sq = _mm256_mul_ps(y, y);
    y = _mm256_mul_ps(y, _mm256_fnmadd_ps(x_half, y_sq, one_point_five));
#endif
    return y;
}
#endif

/* ========== AVX-512 鍚戦噺鍖栫増 (16涓猣loat骞惰) ========== */
#if defined(__AVX512F__) || FAST_RSQRT_HAVE_AVX512
#include <immintrin.h>

FAST_RSQRT_INLINE __m512 fast_rsqrtf_avx512(__m512 x) {
    __m512 y = _mm512_rsqrt14_ps(x);        // 鏇撮珮绮惧害鍒濆€?(14浣?
    __m512 half = _mm512_set1_ps(0.5f);
    __m512 one_point_five = _mm512_set1_ps(1.5f);
    __m512 x_half = _mm512_mul_ps(x, half);
    __m512 y_sq = _mm512_mul_ps(y, y);
    y = _mm512_mul_ps(y, _mm512_fnmadd_ps(x_half, y_sq, one_point_five));
#if RSQRT_NEWTON_STEPS >= 2
    y_sq = _mm512_mul_ps(y, y);
    y = _mm512_mul_ps(y, _mm512_fnmadd_ps(x_half, y_sq, one_point_five));
#endif
    return y;
}
#endif

/* ========== ARM NEON 鍚戦噺鍖栫増 (4涓猣loat骞惰) ========== */
#if defined(__ARM_NEON) || FAST_RSQRT_HAVE_NEON
#include <arm_neon.h>

FAST_RSQRT_INLINE float32x4_t fast_rsqrtf_neon(float32x4_t x) {
    float32x4_t y = vrsqrteq_f32(x);        // NEON 纭欢鍒濆€?
    float32x4_t half = vdupq_n_f32(0.5f);
    float32x4_t x_half = vmulq_f32(x, half);

    // 浣跨敤 vrsqrtsq_f32 杈呭姪鎸囦护杩涜鐗涢】杩唬
    float32x4_t y_sq = vmulq_f32(y, y);
    y = vmulq_f32(y, vrsqrtsq_f32(x_half, y_sq));
#if RSQRT_NEWTON_STEPS >= 2
    y_sq = vmulq_f32(y, y);
    y = vmulq_f32(y, vrsqrtsq_f32(x_half, y_sq));
#endif
    return y;
}
#endif

/* ========== 杩愯鏃?CPU 鍒嗗彂锛氳嚜鍔ㄩ€夋嫨鏈€浼樺疄鐜?========== */
#if !defined(FAST_RSQRT_NO_RUNTIME) && (defined(__x86_64__) || defined(__i386__))
// x86 骞冲彴杩愯鏃跺垎鍙?
FAST_RSQRT_INLINE float fast_rsqrtf(float x) {
    if (FAST_RSQRT_HAVE_SSE) {
        return fast_rsqrtf_sse(x);
    }
    return fast_rsqrtf_software(x);
}
#elif defined(__ARM_NEON) || FAST_RSQRT_HAVE_NEON
// ARM NEON 骞冲彴锛氭爣閲忎篃璧?NEON (鍗曞厓绱犲悜閲?
FAST_RSQRT_INLINE float fast_rsqrtf(float x) {
    float32x4_t vx = vdupq_n_f32(x);
    float32x4_t vy = fast_rsqrtf_neon(vx);
    return vgetq_lane_f32(vy, 0);
}
#else
// 鏃犵‖浠跺姞閫燂細绾蒋浠?
FAST_RSQRT_INLINE float fast_rsqrtf(float x) {
    return fast_rsqrtf_software(x);
}
#endif

/* ========== 渚挎嵎瀹忥細鎵归噺澶勭悊 ========== */
#define fast_rsqrtf_array(arr, n) do { \
    for (size_t _i = 0; _i < (n); ++_i) { \
        (arr)[_i] = fast_rsqrtf((arr)[_i]); \
    } \
} while(0)

#ifdef __cplusplus
}
#endif

#endif /* FAST_RSQRT_H */

/* ============================================================================
 * 模块 3：FAST_TANHF (双曲正切)
 * ============================================================================ */
#ifndef FAST_TANHF_H
#define FAST_TANHF_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ========== 骞冲彴妫€娴嬩笌闄嶇骇 ========== */
#if defined(FAST_TANHF_FALLBACK_STDLIB)
#define FAST_TANHF_USE_STDLIB 1
#elif !defined(__STDC_IEC_559__) && !defined(__IEEE_BIG_ENDIAN) && !defined(__IEEE_LITTLE_ENDIAN)
#define FAST_TANHF_USE_STDLIB 1
#else
#define FAST_TANHF_USE_STDLIB 0
#endif

#if FAST_TANHF_USE_STDLIB
#include <math.h>
#define fast_tanhf tanhf
#ifdef __AVX512F__
#include <immintrin.h>
#define fast_tanhf_avx512 _mm512_tanh_ps
#endif
#else /* 鑷畾涔夊疄鐜?*/

/* ========== 鏍囧噯澶存枃浠?========== */
#ifdef FAST_TANHF_STRICT
#include <errno.h>
#include <fenv.h>
#endif

/* ========== 鍙Щ妞嶆€у畯 ========== */
#ifdef _MSC_VER
#define TANHF_LIKELY(x) (x)
#define TANHF_UNLIKELY(x) (x)
#ifdef FAST_TANHF_NO_FORCE_INLINE
#define TANHF_INLINE static inline
#else
#define TANHF_INLINE __forceinline
#endif
#else
#define TANHF_LIKELY(x) __builtin_expect(!!(x), 1)
#define TANHF_UNLIKELY(x) __builtin_expect(!!(x), 0)
#ifdef FAST_TANHF_NO_FORCE_INLINE
#define TANHF_INLINE static inline
#else
#define TANHF_INLINE static inline __attribute__((always_inline))
#endif
#endif

/* ========== 瀹夊叏妯″紡閰嶇疆 ========== */
#ifdef FAST_TANHF_STRICT
#define TANHF_ERRNO_DOMAIN() \
    do                       \
    {                        \
        errno = EDOM;        \
    } while (0)
#define TANHF_ERRNO_RANGE() \
    do                      \
    {                       \
        errno = ERANGE;     \
    } while (0)
#define TANHF_RAISE_INVALID() feraiseexcept(FE_INVALID)
#define TANHF_RAISE_DIVBYZERO() feraiseexcept(FE_DIVBYZERO)
#define TANHF_RAISE_OVERFLOW() feraiseexcept(FE_OVERFLOW | FE_INEXACT)
#define TANHF_RAISE_UNDERFLOW() feraiseexcept(FE_UNDERFLOW | FE_INEXACT)
#else
#define TANHF_ERRNO_DOMAIN() ((void)0)
#define TANHF_ERRNO_RANGE() ((void)0)
#define TANHF_RAISE_INVALID() ((void)0)
#define TANHF_RAISE_DIVBYZERO() ((void)0)
#define TANHF_RAISE_OVERFLOW() ((void)0)
#define TANHF_RAISE_UNDERFLOW() ((void)0)
#endif

/* ========== 杩愯鏃?CPU 鐗规€ф娴?========== */
#if !defined(FAST_TANHF_NO_RUNTIME_DISPATCH) && defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#include <cpuid.h>
#define TANHF_HAVE_AVX512 (__builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512vl"))
#define TANHF_HAVE_AVX2 __builtin_cpu_supports("avx2")
#define TANHF_HAVE_FMA __builtin_cpu_supports("fma")
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#define TANHF_HAVE_NEON 1
#else
#define TANHF_HAVE_AVX512 0
#define TANHF_HAVE_AVX2 0
#define TANHF_HAVE_NEON 0
#define TANHF_HAVE_FMA 0
#endif

/* ========== 渚濊禆椤瑰鐞?========== */
#ifndef FAST_TANHF_NO_EXPF
/* 浣跨敤 fast_expf 浣滀负鎸囨暟寮曟搸锛岄渶瑕?fast_expf 瀹氫箟 */
#ifndef FAST_EXPF_H
#include "fast_expf.h" /* 宸插湪涓婃枃瀹氫箟锛屽寘鍚椂浼氳烦杩?*/
#endif
#endif

/* ========== 鍐呯疆澶氶」寮忛€艰繎锛堟棤 expf 渚濊禆鏃朵娇鐢級 ========== */
#ifdef FAST_TANHF_RELAXED
/* 5 闃跺椤瑰紡锛岃宸?~1e-6 */
TANHF_INLINE float tanh_poly5(float x)
{
    float x2 = x * x;
    /* 绯绘暟 (tanh(x)/x 鐨勬瀬灏忔瀬澶у椤瑰紡) */
    const float c0 = 1.0f;
    const float c1 = -0.333333333f;
    const float c2 = 0.133333333f;
    const float c3 = -0.053968254f;
    const float c4 = 0.021869489f;
    return x * (c0 + x2 * (c1 + x2 * (c2 + x2 * (c3 + x2 * c4))));
}
#else
/* 7 闃跺椤瑰紡锛岃宸?< 1e-7 */
TANHF_INLINE float tanh_poly7(float x)
{
    float x2 = x * x;
    const float c0 = 1.0f;
    const float c1 = -0.333333333f;
    const float c2 = 0.133333333f;
    const float c3 = -0.053968254f;
    const float c4 = 0.021869489f;
    const float c5 = -0.008863235f;
    const float c6 = 0.003592128f;
    return x * (c0 + x2 * (c1 + x2 * (c2 + x2 * (c3 + x2 * (c4 + x2 * (c5 + x2 * c6))))));
}
#endif

/* ========== 鏍囬噺鏍稿績瀹炵幇 ========== */
TANHF_INLINE float fast_tanhf_scalar(float x)
{
    /* 澶勭悊绗﹀彿锛歵anh 鏄鍑芥暟 */
    union {
        float f;
        uint32_t i;
    } u = {x};
    uint32_t sign = u.i & 0x80000000U;
    float ax = fabsf(x);

    /* 灏忚緭鍏ョ洿鎺ヨ繑鍥?x */
    if (ax < 0.000244140625f)
    { /* 2^{-12} */
        return x;
    }

    /* 澶ц緭鍏ラケ鍜屼负 卤1 */
    if (ax > 19.0f)
    {
        u.i = sign | 0x3F800000U; /* 1.0f 甯︾鍙?*/
        return u.f;
    }

#ifdef FAST_TANHF_NO_EXPF
    float result = tanh_poly7(ax);
    u.f = result;
    u.i |= sign;
    return u.f;
#else
    float e2x = fast_expf(2.0f * ax);
    float y = (e2x - 1.0f) / (e2x + 1.0f);
    u.f = y;
    u.i |= sign;
    return u.f;
#endif
}

/* ========== 鍚戦噺鍖栫増鏈?========== */

/* --- AVX2 鍗曠簿搴?--- */
#if defined(__AVX2__) && TANHF_HAVE_AVX2 && !defined(FAST_TANHF_NO_SIMD)
#include <immintrin.h>
#ifndef FAST_TANHF_NO_EXPF
/* 闇€瑕?fast_expf_avx2 澹版槑 */
#ifndef FAST_EXPF_AVX2_DECLARED
#define FAST_EXPF_AVX2_DECLARED
extern __m256 fast_expf_avx2(__m256 x);
#endif
#endif

TANHF_INLINE __m256 fast_tanhf_avx2(__m256 x)
{
    __m256 ax = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), x); /* fabs */
    __m256 sign = _mm256_and_ps(x, _mm256_set1_ps(-0.0f));
#ifdef FAST_TANHF_NO_EXPF
    /* 浣跨敤澶氶」寮忛€艰繎锛堝悜閲忓寲锛?*/
    __m256 x2 = _mm256_mul_ps(ax, ax);
    const __m256 c0 = _mm256_set1_ps(1.0f);
    const __m256 c1 = _mm256_set1_ps(-0.333333333f);
    const __m256 c2 = _mm256_set1_ps(0.133333333f);
    const __m256 c3 = _mm256_set1_ps(-0.053968254f);
    const __m256 c4 = _mm256_set1_ps(0.021869489f);
    __m256 poly = _mm256_fmadd_ps(c4, x2, c3);
    poly = _mm256_fmadd_ps(poly, x2, c2);
    poly = _mm256_fmadd_ps(poly, x2, c1);
    poly = _mm256_fmadd_ps(poly, x2, c0);
    __m256 result = _mm256_mul_ps(ax, poly);
#else
    __m256 e2x = fast_expf_avx2(_mm256_add_ps(ax, ax)); /* 2*ax */
    __m256 y = _mm256_div_ps(_mm256_sub_ps(e2x, _mm256_set1_ps(1.0f)),
                             _mm256_add_ps(e2x, _mm256_set1_ps(1.0f)));
    __m256 result = _mm256_or_ps(y, sign);
#endif
    return _mm256_or_ps(result, sign);
}
#endif /* __AVX2__ */

/* --- AVX-512 鍗曠簿搴?--- */
#if defined(__AVX512F__) && TANHF_HAVE_AVX512 && !defined(FAST_TANHF_NO_SIMD)
#include <immintrin.h>
#ifndef FAST_TANHF_NO_EXPF
#ifndef FAST_EXPF_AVX512_DECLARED
#define FAST_EXPF_AVX512_DECLARED
extern __m512 fast_expf_avx512(__m512 x);
#endif
#endif

TANHF_INLINE __m512 fast_tanhf_avx512(__m512 x)
{
    __m512 ax = _mm512_abs_ps(x);
    __m512 sign = _mm512_and_ps(x, _mm512_set1_ps(-0.0f));
#ifdef FAST_TANHF_NO_EXPF
    __m512 x2 = _mm512_mul_ps(ax, ax);
    const __m512 c0 = _mm512_set1_ps(1.0f);
    const __m512 c1 = _mm512_set1_ps(-0.333333333f);
    const __m512 c2 = _mm512_set1_ps(0.133333333f);
    const __m512 c3 = _mm512_set1_ps(-0.053968254f);
    const __m512 c4 = _mm512_set1_ps(0.021869489f);
    __m512 poly = _mm512_fmadd_ps(c4, x2, c3);
    poly = _mm512_fmadd_ps(poly, x2, c2);
    poly = _mm512_fmadd_ps(poly, x2, c1);
    poly = _mm512_fmadd_ps(poly, x2, c0);
    __m512 result = _mm512_mul_ps(ax, poly);
#else
    __m512 e2x = fast_expf_avx512(_mm512_add_ps(ax, ax));
    __m512 y = _mm512_div_ps(_mm512_sub_ps(e2x, _mm512_set1_ps(1.0f)),
                             _mm512_add_ps(e2x, _mm512_set1_ps(1.0f)));
    __m512 result = _mm512_or_ps(y, sign);
#endif
    return _mm512_or_ps(result, sign);
}
#endif /* __AVX512F__ */

/* --- ARM NEON 鍗曠簿搴?--- */
#if defined(__ARM_NEON) && TANHF_HAVE_NEON && !defined(FAST_TANHF_NO_SIMD)
#include <arm_neon.h>
#ifndef FAST_TANHF_NO_EXPF
#ifndef FAST_EXPF_NEON_DECLARED
#define FAST_EXPF_NEON_DECLARED
extern float32x4_t fast_expf_neon(float32x4_t x);
#endif
#endif

TANHF_INLINE float32x4_t fast_tanhf_neon(float32x4_t x)
{
    float32x4_t ax = vabsq_f32(x);
    uint32x4_t sign = vandq_u32(vreinterpretq_u32_f32(x), vdupq_n_u32(0x80000000U));
#ifdef FAST_TANHF_NO_EXPF
    float32x4_t x2 = vmulq_f32(ax, ax);
    const float32x4_t c0 = vdupq_n_f32(1.0f);
    const float32x4_t c1 = vdupq_n_f32(-0.333333333f);
    const float32x4_t c2 = vdupq_n_f32(0.133333333f);
    const float32x4_t c3 = vdupq_n_f32(-0.053968254f);
    const float32x4_t c4 = vdupq_n_f32(0.021869489f);
    float32x4_t poly = vmlaq_f32(c3, c4, x2);
    poly = vmlaq_f32(c2, poly, x2);
    poly = vmlaq_f32(c1, poly, x2);
    poly = vmlaq_f32(c0, poly, x2);
    float32x4_t result = vmulq_f32(ax, poly);
#else
    float32x4_t two = vdupq_n_f32(2.0f);
    float32x4_t e2x = fast_expf_neon(vmulq_f32(ax, two));
    float32x4_t one = vdupq_n_f32(1.0f);
    float32x4_t y = vdivq_f32(vsubq_f32(e2x, one), vaddq_f32(e2x, one));
    float32x4_t result = vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(y), sign));
#endif
    return vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(result), sign));
}
#endif /* __ARM_NEON */

/* ========== 杩愯鏃?CPU 鍒嗗彂 ========== */
#if !defined(FAST_TANHF_NO_RUNTIME_DISPATCH) && (defined(__x86_64__) || defined(__i386__))
TANHF_INLINE float fast_tanhf(float x)
{
    if (TANHF_HAVE_AVX512)
    {
        __m128 vx = _mm_set_ss(x);
        __m512 vx512 = _mm512_broadcastss_ps(vx);
        __m512 vy = fast_tanhf_avx512(vx512);
        return _mm_cvtss_f32(_mm512_castps512_ps128(vy));
    }
    else if (TANHF_HAVE_AVX2)
    {
        __m128 vx = _mm_set_ss(x);
        __m256 vx256 = _mm256_broadcastss_ps(vx);
        __m256 vy = fast_tanhf_avx2(vx256);
        return _mm_cvtss_f32(_mm256_castps256_ps128(vy));
    }
    else
    {
        return fast_tanhf_scalar(x);
    }
}
#elif defined(__ARM_NEON) && TANHF_HAVE_NEON
TANHF_INLINE float fast_tanhf(float x)
{
    float32x4_t vx = vdupq_n_f32(x);
    float32x4_t vy = fast_tanhf_neon(vx);
    return vgetq_lane_f32(vy, 0);
}
#else
TANHF_INLINE float fast_tanhf(float x)
{
    return fast_tanhf_scalar(x);
}
#endif

#endif /* FAST_TANHF_USE_STDLIB */
#endif /* FAST_TANHF_H */

/* 提前声明向量函数，供后续模块使用 */
#ifndef FAST_TANHF_AVX2_DECLARED
#define FAST_TANHF_AVX2_DECLARED
#endif
#ifndef FAST_TANHF_AVX512_DECLARED
#define FAST_TANHF_AVX512_DECLARED
#endif
#ifndef FAST_TANHF_NEON_DECLARED
#define FAST_TANHF_NEON_DECLARED
#endif

/* ============================================================================
 * 模块 4：FAST_SIGMOIDF (Sigmoid)
 * ============================================================================ */
#ifndef FAST_SIGMOIDF_H
#define FAST_SIGMOIDF_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ========== 骞冲彴妫€娴嬩笌闄嶇骇 ========== */
#if defined(FAST_SIGMOIDF_FALLBACK_STDLIB)
#define FAST_SIGMOIDF_USE_STDLIB 1
#else
#define FAST_SIGMOIDF_USE_STDLIB 0
#endif

#if FAST_SIGMOIDF_USE_STDLIB
#include <math.h>
#define fast_sigmoidf(x) (1.0f / (1.0f + expf(-(x))))
#else

/* ========== 鍙Щ妞嶆€у畯 ========== */
#ifdef _MSC_VER
#define SIGMOIDF_LIKELY(x) (x)
#define SIGMOIDF_UNLIKELY(x) (x)
#define SIGMOIDF_INLINE __forceinline
#else
#define SIGMOIDF_LIKELY(x) __builtin_expect(!!(x), 1)
#define SIGMOIDF_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define SIGMOIDF_INLINE static inline __attribute__((always_inline))
#endif

/* ========== 杩愯鏃?CPU 鐗规€ф娴?========== */
#if !defined(FAST_SIGMOIDF_NO_RUNTIME_DISPATCH) && defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#include <cpuid.h>
#define SIGMOIDF_HAVE_AVX512 (__builtin_cpu_supports("avx512f"))
#define SIGMOIDF_HAVE_AVX2 __builtin_cpu_supports("avx2")
#elif defined(__AVX512F__)
#define SIGMOIDF_HAVE_AVX512 1
#elif defined(__AVX2__)
#define SIGMOIDF_HAVE_AVX2 1
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#define SIGMOIDF_HAVE_NEON 1
#else
#define SIGMOIDF_HAVE_AVX512 0
#define SIGMOIDF_HAVE_AVX2 0
#define SIGMOIDF_HAVE_NEON 0
#endif

/* 渚濊禆 fast_expf.h */
#ifndef FAST_EXPF_H
#include "fast_expf.h"
#endif

/* ========== 鏍囬噺鏍稿績瀹炵幇 ========== */
SIGMOIDF_INLINE float fast_sigmoidf_scalar(float x)
{
    if (x >= 0.0f)
    {
        return 1.0f / (1.0f + fast_expf(-x));
    }
    else
    {
        float ex = fast_expf(x);
        return ex / (1.0f + ex);
    }
}

/* ========== 鍚戦噺鍖栫増鏈?========== */

/* --- AVX2 --- */
#if defined(__AVX2__) && SIGMOIDF_HAVE_AVX2 && !defined(FAST_SIGMOIDF_NO_SIMD)
#include <immintrin.h>
#ifndef FAST_EXPF_AVX2_DECLARED
#define FAST_EXPF_AVX2_DECLARED
extern __m256 fast_expf_avx2(__m256 x);
#endif

SIGMOIDF_INLINE __m256 fast_sigmoidf_avx2(__m256 x)
{
    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);
    __m256 mask = _mm256_cmp_ps(x, zero, _CMP_GE_OQ);
    __m256 neg_x = _mm256_sub_ps(zero, x);
    __m256 exp_val = fast_expf_avx2(_mm256_blendv_ps(neg_x, x, mask));
    __m256 denom = _mm256_add_ps(one, exp_val);
    __m256 pos_result = _mm256_div_ps(one, denom);
    __m256 neg_result = _mm256_div_ps(exp_val, denom);
    return _mm256_blendv_ps(neg_result, pos_result, mask);
}
#endif /* __AVX2__ */

/* --- AVX-512 --- */
#if defined(__AVX512F__) && SIGMOIDF_HAVE_AVX512 && !defined(FAST_SIGMOIDF_NO_SIMD)
#include <immintrin.h>
#ifndef FAST_EXPF_AVX512_DECLARED
#define FAST_EXPF_AVX512_DECLARED
extern __m512 fast_expf_avx512(__m512 x);
#endif

SIGMOIDF_INLINE __m512 fast_sigmoidf_avx512(__m512 x)
{
    __m512 zero = _mm512_setzero_ps();
    __m512 one = _mm512_set1_ps(1.0f);
    __mmask16 mask = _mm512_cmp_ps_mask(x, zero, _CMP_GE_OQ);
    __m512 neg_x = _mm512_sub_ps(zero, x);
    __m512 exp_val = fast_expf_avx512(_mm512_mask_blend_ps(mask, neg_x, x));
    __m512 denom = _mm512_add_ps(one, exp_val);
    __m512 pos_result = _mm512_div_ps(one, denom);
    __m512 neg_result = _mm512_div_ps(exp_val, denom);
    return _mm512_mask_blend_ps(mask, neg_result, pos_result);
}
#endif /* __AVX512F__ */

/* --- NEON --- */
#if defined(__ARM_NEON) && SIGMOIDF_HAVE_NEON && !defined(FAST_SIGMOIDF_NO_SIMD)
#include <arm_neon.h>
#ifndef FAST_EXPF_NEON_DECLARED
#define FAST_EXPF_NEON_DECLARED
extern float32x4_t fast_expf_neon(float32x4_t x);
#endif

SIGMOIDF_INLINE float32x4_t fast_sigmoidf_neon(float32x4_t x)
{
    float32x4_t zero = vdupq_n_f32(0.0f);
    float32x4_t one = vdupq_n_f32(1.0f);
    uint32x4_t mask = vcgeq_f32(x, zero);
    float32x4_t neg_x = vsubq_f32(zero, x);
    float32x4_t exp_val = fast_expf_neon(vbslq_f32(mask, x, neg_x));
    float32x4_t denom = vaddq_f32(one, exp_val);
    float32x4_t pos_result = vdivq_f32(one, denom);
    float32x4_t neg_result = vdivq_f32(exp_val, denom);
    return vbslq_f32(mask, pos_result, neg_result);
}
#endif /* __ARM_NEON */

/* ========== 杩愯鏃?CPU 鍒嗗彂 ========== */
#if !defined(FAST_SIGMOIDF_NO_RUNTIME_DISPATCH) && (defined(__x86_64__) || defined(__i386__))
SIGMOIDF_INLINE float fast_sigmoidf(float x)
{
    if (SIGMOIDF_HAVE_AVX512)
    {
        __m128 vx = _mm_set_ss(x);
        __m512 vx512 = _mm512_broadcastss_ps(vx);
        __m512 vy = fast_sigmoidf_avx512(vx512);
        return _mm_cvtss_f32(_mm512_castps512_ps128(vy));
    }
    else if (SIGMOIDF_HAVE_AVX2)
    {
        __m128 vx = _mm_set_ss(x);
        __m256 vx256 = _mm256_broadcastss_ps(vx);
        __m256 vy = fast_sigmoidf_avx2(vx256);
        return _mm_cvtss_f32(_mm256_castps256_ps128(vy));
    }
    else
    {
        return fast_sigmoidf_scalar(x);
    }
}
#elif defined(__ARM_NEON) && SIGMOIDF_HAVE_NEON
SIGMOIDF_INLINE float fast_sigmoidf(float x)
{
    float32x4_t vx = vdupq_n_f32(x);
    float32x4_t vy = fast_sigmoidf_neon(vx);
    return vgetq_lane_f32(vy, 0);
}
#else
SIGMOIDF_INLINE float fast_sigmoidf(float x)
{
    return fast_sigmoidf_scalar(x);
}
#endif

#endif /* FAST_SIGMOIDF_USE_STDLIB */
#endif /* FAST_SIGMOIDF_H */

/* 提前声明向量函数 */
#ifndef FAST_SIGMOIDF_AVX2_DECLARED
#define FAST_SIGMOIDF_AVX2_DECLARED
#endif
#ifndef FAST_SIGMOIDF_AVX512_DECLARED
#define FAST_SIGMOIDF_AVX512_DECLARED
#endif
#ifndef FAST_SIGMOIDF_NEON_DECLARED
#define FAST_SIGMOIDF_NEON_DECLARED
#endif

/* ============================================================================
 * 模块 5：FAST_GELUF (GELU)
 * ============================================================================ */
#ifndef FAST_GELUF_H
#define FAST_GELUF_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ========== 骞冲彴妫€娴嬩笌闄嶇骇 ========== */
#if defined(FAST_GELUF_FALLBACK_STDLIB)
#define FAST_GELUF_USE_STDLIB 1
#else
#define FAST_GELUF_USE_STDLIB 0
#endif

#if FAST_GELUF_USE_STDLIB
#include <math.h>
#define fast_geluf(x) (0.5f * (x) * (1.0f + tanhf(0.7978845608028654f * ((x) + 0.044715f * (x) * (x) * (x)))))
#else

/* ========== 鍙Щ妞嶆€у畯 ========== */
#ifdef _MSC_VER
#define GELUF_LIKELY(x) (x)
#define GELUF_UNLIKELY(x) (x)
#define GELUF_INLINE __forceinline
#else
#define GELUF_LIKELY(x) __builtin_expect(!!(x), 1)
#define GELUF_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define GELUF_INLINE static inline __attribute__((always_inline))
#endif

/* ========== 杩愯鏃?CPU 鐗规€ф娴?========== */
#if !defined(FAST_GELUF_NO_RUNTIME_DISPATCH) && defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#include <cpuid.h>
#define GELUF_HAVE_AVX512 (__builtin_cpu_supports("avx512f"))
#define GELUF_HAVE_AVX2 __builtin_cpu_supports("avx2")
#elif defined(__AVX512F__)
#define GELUF_HAVE_AVX512 1
#elif defined(__AVX2__)
#define GELUF_HAVE_AVX2 1
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#define GELUF_HAVE_NEON 1
#else
#define GELUF_HAVE_AVX512 0
#define GELUF_HAVE_AVX2 0
#define GELUF_HAVE_NEON 0
#endif

/* 渚濊禆 fast_tanhf.h */
#ifndef FAST_TANHF_H
#include "fast_tanhf.h"
#endif

/* ========== 甯搁噺 ========== */
static const float GELUF_SQRT_2_OVER_PI = 0.7978845608028654f;
static const float GELUF_COEFF = 0.044715f;

/* ========== 鏍囬噺鏍稿績瀹炵幇 ========== */
GELUF_INLINE float fast_geluf_scalar(float x)
{
    float x2 = x * x;
    float x3 = x2 * x;
    float inner = GELUF_SQRT_2_OVER_PI * (x + GELUF_COEFF * x3);
    float tanh_inner = fast_tanhf(inner);
    return 0.5f * x * (1.0f + tanh_inner);
}

/* ========== 鍚戦噺鍖栫増鏈?========== */
#if defined(__AVX2__) && GELUF_HAVE_AVX2 && !defined(FAST_GELUF_NO_SIMD)
#include <immintrin.h>
#ifndef FAST_TANHF_AVX2_DECLARED
#define FAST_TANHF_AVX2_DECLARED
extern __m256 fast_tanhf_avx2(__m256 x);
#endif

GELUF_INLINE __m256 fast_geluf_avx2(__m256 x)
{
    __m256 x2 = _mm256_mul_ps(x, x);
    __m256 x3 = _mm256_mul_ps(x2, x);
    __m256 coeff = _mm256_set1_ps(GELUF_COEFF);
    __m256 sqrt_2_over_pi = _mm256_set1_ps(GELUF_SQRT_2_OVER_PI);
    __m256 inner = _mm256_mul_ps(sqrt_2_over_pi,
                                 _mm256_add_ps(x, _mm256_mul_ps(coeff, x3)));
    __m256 tanh_inner = fast_tanhf_avx2(inner);
    __m256 half = _mm256_set1_ps(0.5f);
    __m256 one = _mm256_set1_ps(1.0f);
    return _mm256_mul_ps(_mm256_mul_ps(half, x),
                         _mm256_add_ps(one, tanh_inner));
}
#endif /* __AVX2__ */

#if defined(__AVX512F__) && GELUF_HAVE_AVX512 && !defined(FAST_GELUF_NO_SIMD)
#include <immintrin.h>
#ifndef FAST_TANHF_AVX512_DECLARED
#define FAST_TANHF_AVX512_DECLARED
extern __m512 fast_tanhf_avx512(__m512 x);
#endif

GELUF_INLINE __m512 fast_geluf_avx512(__m512 x)
{
    __m512 x2 = _mm512_mul_ps(x, x);
    __m512 x3 = _mm512_mul_ps(x2, x);
    __m512 coeff = _mm512_set1_ps(GELUF_COEFF);
    __m512 sqrt_2_over_pi = _mm512_set1_ps(GELUF_SQRT_2_OVER_PI);
    __m512 inner = _mm512_mul_ps(sqrt_2_over_pi,
                                 _mm512_add_ps(x, _mm512_mul_ps(coeff, x3)));
    __m512 tanh_inner = fast_tanhf_avx512(inner);
    __m512 half = _mm512_set1_ps(0.5f);
    __m512 one = _mm512_set1_ps(1.0f);
    return _mm512_mul_ps(_mm512_mul_ps(half, x),
                         _mm512_add_ps(one, tanh_inner));
}
#endif /* __AVX512F__ */

#if defined(__ARM_NEON) && GELUF_HAVE_NEON && !defined(FAST_GELUF_NO_SIMD)
#include <arm_neon.h>
#ifndef FAST_TANHF_NEON_DECLARED
#define FAST_TANHF_NEON_DECLARED
extern float32x4_t fast_tanhf_neon(float32x4_t x);
#endif

GELUF_INLINE float32x4_t fast_geluf_neon(float32x4_t x)
{
    float32x4_t x2 = vmulq_f32(x, x);
    float32x4_t x3 = vmulq_f32(x2, x);
    float32x4_t coeff = vdupq_n_f32(GELUF_COEFF);
    float32x4_t sqrt_2_over_pi = vdupq_n_f32(GELUF_SQRT_2_OVER_PI);
    float32x4_t inner = vmulq_f32(sqrt_2_over_pi,
                                  vaddq_f32(x, vmulq_f32(coeff, x3)));
    float32x4_t tanh_inner = fast_tanhf_neon(inner);
    float32x4_t half = vdupq_n_f32(0.5f);
    float32x4_t one = vdupq_n_f32(1.0f);
    return vmulq_f32(vmulq_f32(half, x),
                     vaddq_f32(one, tanh_inner));
}
#endif /* __ARM_NEON */

/* ========== 杩愯鏃?CPU 鍒嗗彂 ========== */
#if !defined(FAST_GELUF_NO_RUNTIME_DISPATCH) && (defined(__x86_64__) || defined(__i386__))
GELUF_INLINE float fast_geluf(float x)
{
    if (GELUF_HAVE_AVX512)
    {
        __m128 vx = _mm_set_ss(x);
        __m512 vx512 = _mm512_broadcastss_ps(vx);
        __m512 vy = fast_geluf_avx512(vx512);
        return _mm_cvtss_f32(_mm512_castps512_ps128(vy));
    }
    else if (GELUF_HAVE_AVX2)
    {
        __m128 vx = _mm_set_ss(x);
        __m256 vx256 = _mm256_broadcastss_ps(vx);
        __m256 vy = fast_geluf_avx2(vx256);
        return _mm_cvtss_f32(_mm256_castps256_ps128(vy));
    }
    else
    {
        return fast_geluf_scalar(x);
    }
}
#elif defined(__ARM_NEON) && GELUF_HAVE_NEON
GELUF_INLINE float fast_geluf(float x)
{
    float32x4_t vx = vdupq_n_f32(x);
    float32x4_t vy = fast_geluf_neon(vx);
    return vgetq_lane_f32(vy, 0);
}
#else
GELUF_INLINE float fast_geluf(float x)
{
    return fast_geluf_scalar(x);
}
#endif

#endif /* FAST_GELUF_USE_STDLIB */
#endif /* FAST_GELUF_H */

/* ============================================================================
 * 模块 6：FAST_SWISHF (Swish)
 * ============================================================================ */
#ifndef FAST_SWISHF_H
#define FAST_SWISHF_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ========== 骞冲彴妫€娴嬩笌闄嶇骇 ========== */
#if defined(FAST_SWISHF_FALLBACK_STDLIB)
#define FAST_SWISHF_USE_STDLIB 1
#else
#define FAST_SWISHF_USE_STDLIB 0
#endif

#if FAST_SWISHF_USE_STDLIB
#include <math.h>
#define fast_swishf(x) ((x) / (1.0f + expf(-(x))))
#else

/* ========== 鍙Щ妞嶆€у畯 ========== */
#ifdef _MSC_VER
#define SWISHF_LIKELY(x) (x)
#define SWISHF_UNLIKELY(x) (x)
#define SWISHF_INLINE __forceinline
#else
#define SWISHF_LIKELY(x) __builtin_expect(!!(x), 1)
#define SWISHF_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define SWISHF_INLINE static inline __attribute__((always_inline))
#endif

/* ========== 杩愯鏃?CPU 鐗规€ф娴?========== */
#if !defined(FAST_SWISHF_NO_RUNTIME_DISPATCH) && defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#include <cpuid.h>
#define SWISHF_HAVE_AVX512 (__builtin_cpu_supports("avx512f"))
#define SWISHF_HAVE_AVX2 __builtin_cpu_supports("avx2")
#elif defined(__AVX512F__)
#define SWISHF_HAVE_AVX512 1
#elif defined(__AVX2__)
#define SWISHF_HAVE_AVX2 1
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#define SWISHF_HAVE_NEON 1
#else
#define SWISHF_HAVE_AVX512 0
#define SWISHF_HAVE_AVX2 0
#define SWISHF_HAVE_NEON 0
#endif

/* 渚濊禆 fast_sigmoidf.h */
#ifndef FAST_SIGMOIDF_H
#include "fast_sigmoidf.h"
#endif

/* ========== 鏍囬噺鏍稿績瀹炵幇 ========== */
SWISHF_INLINE float fast_swishf_scalar(float x)
{
    return x * fast_sigmoidf(x);
}

/* ========== 鍚戦噺鍖栫増鏈?========== */
#if defined(__AVX2__) && SWISHF_HAVE_AVX2 && !defined(FAST_SWISHF_NO_SIMD)
#include <immintrin.h>
#ifndef FAST_SIGMOIDF_AVX2_DECLARED
#define FAST_SIGMOIDF_AVX2_DECLARED
extern __m256 fast_sigmoidf_avx2(__m256 x);
#endif

SWISHF_INLINE __m256 fast_swishf_avx2(__m256 x)
{
    return _mm256_mul_ps(x, fast_sigmoidf_avx2(x));
}
#endif /* __AVX2__ */

#if defined(__AVX512F__) && SWISHF_HAVE_AVX512 && !defined(FAST_SWISHF_NO_SIMD)
#include <immintrin.h>
#ifndef FAST_SIGMOIDF_AVX512_DECLARED
#define FAST_SIGMOIDF_AVX512_DECLARED
extern __m512 fast_sigmoidf_avx512(__m512 x);
#endif

SWISHF_INLINE __m512 fast_swishf_avx512(__m512 x)
{
    return _mm512_mul_ps(x, fast_sigmoidf_avx512(x));
}
#endif /* __AVX512F__ */

#if defined(__ARM_NEON) && SWISHF_HAVE_NEON && !defined(FAST_SWISHF_NO_SIMD)
#include <arm_neon.h>
#ifndef FAST_SIGMOIDF_NEON_DECLARED
#define FAST_SIGMOIDF_NEON_DECLARED
extern float32x4_t fast_sigmoidf_neon(float32x4_t x);
#endif

SWISHF_INLINE float32x4_t fast_swishf_neon(float32x4_t x)
{
    return vmulq_f32(x, fast_sigmoidf_neon(x));
}
#endif /* __ARM_NEON */

/* ========== 杩愯鏃?CPU 鍒嗗彂 ========== */
#if !defined(FAST_SWISHF_NO_RUNTIME_DISPATCH) && (defined(__x86_64__) || defined(__i386__))
SWISHF_INLINE float fast_swishf(float x)
{
    if (SWISHF_HAVE_AVX512)
    {
        __m128 vx = _mm_set_ss(x);
        __m512 vx512 = _mm512_broadcastss_ps(vx);
        __m512 vy = fast_swishf_avx512(vx512);
        return _mm_cvtss_f32(_mm512_castps512_ps128(vy));
    }
    else if (SWISHF_HAVE_AVX2)
    {
        __m128 vx = _mm_set_ss(x);
        __m256 vx256 = _mm256_broadcastss_ps(vx);
        __m256 vy = fast_swishf_avx2(vx256);
        return _mm_cvtss_f32(_mm256_castps256_ps128(vy));
    }
    else
    {
        return fast_swishf_scalar(x);
    }
}
#elif defined(__ARM_NEON) && SWISHF_HAVE_NEON
SWISHF_INLINE float fast_swishf(float x)
{
    float32x4_t vx = vdupq_n_f32(x);
    float32x4_t vy = fast_swishf_neon(vx);
    return vgetq_lane_f32(vy, 0);
}
#else
SWISHF_INLINE float fast_swishf(float x)
{
    return fast_swishf_scalar(x);
}
#endif

#endif /* FAST_SWISHF_USE_STDLIB */
#endif /* FAST_SWISHF_H */

/* ============================================================================
 * 模块 7：FAST_SOFTMAXF (Softmax)
 * ============================================================================ */
#ifndef FAST_SOFTMAXF_H
#define FAST_SOFTMAXF_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 骞冲彴妫€娴?========== */
#ifndef FAST_SOFTMAXF_NO_RUNTIME_DISPATCH
    #if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
        #include <cpuid.h>
        #define SOFTMAX_HAVE_AVX512 (__builtin_cpu_supports("avx512f"))
        #define SOFTMAX_HAVE_AVX2   __builtin_cpu_supports("avx2")
    #elif defined(__AVX512F__)
        #define SOFTMAX_HAVE_AVX512 1
    #elif defined(__AVX2__)
        #define SOFTMAX_HAVE_AVX2   1
    #else
        #define SOFTMAX_HAVE_AVX512 0
        #define SOFTMAX_HAVE_AVX2   0
    #endif
#endif

/* ========== 鍙Щ妞嶆€у畯 ========== */
#ifdef _MSC_VER
    #define SOFTMAX_INLINE static __forceinline
#else
    #define SOFTMAX_INLINE static inline __attribute__((always_inline))
#endif

/* 鎸囨暟鍑芥暟閫夋嫨 */
#ifdef FAST_SOFTMAX_USE_EXPF
    #ifndef FAST_EXPF_H
        #include "fast_expf.h"
    #endif
    #define SOFTMAX_EXP fast_expf
#else
    #define SOFTMAX_EXP expf
#endif

/* ========== 鏍囬噺瀹炵幇 ========== */
SOFTMAX_INLINE void fast_softmaxf_scalar(float* x, int n) {
    /* 鎵惧嚭鏈€澶у€间互闃叉鎸囨暟婧㈠嚭 */
    float max_val = x[0];
    for (int i = 1; i < n; ++i) {
        if (x[i] > max_val) max_val = x[i];
    }

    /* 璁＄畻 exp(x[i] - max) 骞舵眰鍜?*/
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) {
        x[i] = SOFTMAX_EXP(x[i] - max_val);
        sum += x[i];
    }

    /* 褰掍竴鍖?*/
    float inv_sum = 1.0f / sum;
    for (int i = 0; i < n; ++i) {
        x[i] *= inv_sum;
    }
}

/* ========== AVX2 鍚戦噺鍖栧疄鐜?========== */
#if defined(__AVX2__) && SOFTMAX_HAVE_AVX2 && !defined(FAST_SOFTMAX_NO_SIMD)
#include <immintrin.h>
#ifdef FAST_SOFTMAX_USE_EXPF
#ifndef FAST_EXPF_AVX2_DECLARED
#define FAST_EXPF_AVX2_DECLARED
extern __m256 fast_expf_avx2(__m256 x);
#endif
#define SOFTMAX_EXP_VEC fast_expf_avx2
#else
/* 濡傛灉鏍囧噯搴撴病鏈夋彁渚?_mm256_exp_ps锛屽洖閫€鍒版爣閲忓惊鐜?*/
#define SOFTMAX_EXP_VEC _mm256_exp_ps
#ifndef _mm256_exp_ps
#define SOFTMAX_FALLBACK_SCALAR_EXP
#endif
#endif

SOFTMAX_INLINE void fast_softmaxf_avx2(float* x, int n) {
    /* 鎵惧嚭鏈€澶у€硷紙鏍囬噺鎴栧悜閲忓寲褰掔害锛?*/
    float max_val = x[0];
    int i = 0;
    for (; i + 7 < n; i += 8) {
        __m256 v = _mm256_loadu_ps(&x[i]);
        __m256 maxv = v;
        /* 绠€鍗曟爣閲忔瘮杈冧繚鎸佷唬鐮佹竻鏅帮紝瀹為檯鍙敤 _mm256_max_ps 杩涜鍚戦噺褰掔害 */
        float tmp[8];
        _mm256_storeu_ps(tmp, v);
        for (int k = 0; k < 8; ++k) if (tmp[k] > max_val) max_val = tmp[k];
    }
    for (; i < n; ++i) {
        if (x[i] > max_val) max_val = x[i];
    }

    __m256 max_vec = _mm256_set1_ps(max_val);
    __m256 sum_vec = _mm256_setzero_ps();

    /* 涓诲惊鐜細璁＄畻 exp 骞剁疮鍔?*/
    i = 0;
    for (; i + 7 < n; i += 8) {
        __m256 v = _mm256_loadu_ps(&x[i]);
        v = _mm256_sub_ps(v, max_vec);
#ifdef SOFTMAX_FALLBACK_SCALAR_EXP
        /* 鍥為€€鍒版爣閲?exp */
        float tmp[8];
        _mm256_storeu_ps(tmp, v);
        for (int k = 0; k < 8; ++k) tmp[k] = SOFTMAX_EXP(tmp[k]);
        v = _mm256_loadu_ps(tmp);
#else
        v = SOFTMAX_EXP_VEC(v);
#endif
        _mm256_storeu_ps(&x[i], v);
        sum_vec = _mm256_add_ps(sum_vec, v);
    }

    /* 姘村钩姹傚拰 sum_vec */
    float sum = 0.0f;
    float tmp[8];
    _mm256_storeu_ps(tmp, sum_vec);
    for (int k = 0; k < 8; ++k) sum += tmp[k];

    /* 澶勭悊鍓╀綑鍏冪礌 */
    for (; i < n; ++i) {
        x[i] = SOFTMAX_EXP(x[i] - max_val);
        sum += x[i];
    }

    /* 褰掍竴鍖?*/
    float inv_sum = 1.0f / sum;
    __m256 inv_sum_vec = _mm256_set1_ps(inv_sum);
    i = 0;
    for (; i + 7 < n; i += 8) {
        __m256 v = _mm256_loadu_ps(&x[i]);
        v = _mm256_mul_ps(v, inv_sum_vec);
        _mm256_storeu_ps(&x[i], v);
    }
    for (; i < n; ++i) {
        x[i] *= inv_sum;
    }
}
#endif /* __AVX2__ */

/* ========== AVX-512 鍚戦噺鍖栧疄鐜?========== */
#if defined(__AVX512F__) && SOFTMAX_HAVE_AVX512 && !defined(FAST_SOFTMAX_NO_SIMD)
#include <immintrin.h>
#ifdef FAST_SOFTMAX_USE_EXPF
#ifndef FAST_EXPF_AVX512_DECLARED
#define FAST_EXPF_AVX512_DECLARED
extern __m512 fast_expf_avx512(__m512 x);
#endif
#define SOFTMAX_EXP_VEC512 fast_expf_avx512
#else
#define SOFTMAX_EXP_VEC512 _mm512_exp_ps
#ifndef _mm512_exp_ps
#define SOFTMAX_FALLBACK_SCALAR_EXP512
#endif
#endif

SOFTMAX_INLINE void fast_softmaxf_avx512(float* x, int n) {
    /* 鎵惧嚭鏈€澶у€?*/
    float max_val = x[0];
    int i = 0;
    for (; i + 15 < n; i += 16) {
        __m512 v = _mm512_loadu_ps(&x[i]);
        float tmp[16];
        _mm512_storeu_ps(tmp, v);
        for (int k = 0; k < 16; ++k) if (tmp[k] > max_val) max_val = tmp[k];
    }
    for (; i < n; ++i) {
        if (x[i] > max_val) max_val = x[i];
    }

    __m512 max_vec = _mm512_set1_ps(max_val);
    __m512 sum_vec = _mm512_setzero_ps();

    i = 0;
    for (; i + 15 < n; i += 16) {
        __m512 v = _mm512_loadu_ps(&x[i]);
        v = _mm512_sub_ps(v, max_vec);
#ifdef SOFTMAX_FALLBACK_SCALAR_EXP512
        float tmp[16];
        _mm512_storeu_ps(tmp, v);
        for (int k = 0; k < 16; ++k) tmp[k] = SOFTMAX_EXP(tmp[k]);
        v = _mm512_loadu_ps(tmp);
#else
        v = SOFTMAX_EXP_VEC512(v);
#endif
        _mm512_storeu_ps(&x[i], v);
        sum_vec = _mm512_add_ps(sum_vec, v);
    }

    /* 姘村钩姹傚拰 sum_vec */
    float sum = _mm512_reduce_add_ps(sum_vec);

    /* 澶勭悊鍓╀綑鍏冪礌 */
    for (; i < n; ++i) {
        x[i] = SOFTMAX_EXP(x[i] - max_val);
        sum += x[i];
    }

    /* 褰掍竴鍖?*/
    float inv_sum = 1.0f / sum;
    __m512 inv_sum_vec = _mm512_set1_ps(inv_sum);
    i = 0;
    for (; i + 15 < n; i += 16) {
        __m512 v = _mm512_loadu_ps(&x[i]);
        v = _mm512_mul_ps(v, inv_sum_vec);
        _mm512_storeu_ps(&x[i], v);
    }
    for (; i < n; ++i) {
        x[i] *= inv_sum;
    }
}
#endif /* __AVX512F__ */

/* ========== 杩愯鏃?CPU 鍒嗗彂 ========== */
SOFTMAX_INLINE void fast_softmaxf(float* x, int n) {
#if !defined(FAST_SOFTMAX_NO_RUNTIME_DISPATCH) && (defined(__x86_64__) || defined(__i386__))
    if (SOFTMAX_HAVE_AVX512) {
        fast_softmaxf_avx512(x, n);
    } else if (SOFTMAX_HAVE_AVX2) {
        fast_softmaxf_avx2(x, n);
    } else {
        fast_softmaxf_scalar(x, n);
    }
#elif defined(__AVX512F__) && !defined(FAST_SOFTMAX_NO_SIMD)
    fast_softmaxf_avx512(x, n);
#elif defined(__AVX2__) && !defined(FAST_SOFTMAX_NO_SIMD)
    fast_softmaxf_avx2(x, n);
#else
    fast_softmaxf_scalar(x, n);
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* FAST_SOFTMAXF_H */

/* ============================================================================
 * 模块 8：FAST_SINCOSF (正弦/余弦)
 * ============================================================================ */
#ifndef FAST_SINCOSF_H
#define FAST_SINCOSF_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ========== 骞冲彴妫€娴嬩笌闄嶇骇 ========== */
#if defined(FAST_SINCOSF_FALLBACK_STDLIB)
#define FAST_SINCOSF_USE_STDLIB 1
#elif !defined(__STDC_IEC_559__) && !defined(__IEEE_BIG_ENDIAN) && !defined(__IEEE_LITTLE_ENDIAN)
#define FAST_SINCOSF_USE_STDLIB 1
#else
#define FAST_SINCOSF_USE_STDLIB 0
#endif

#if FAST_SINCOSF_USE_STDLIB
#include <math.h>
#define fast_sinf sinf
#define fast_cosf cosf
#define fast_sincosf(x, s, c) \
    do                        \
    {                         \
        *(s) = sinf(x);       \
        *(c) = cosf(x);       \
    } while (0)
#ifdef __AVX512F__
#include <immintrin.h>
#define fast_sinf_avx512 _mm512_sin_ps
#define fast_cosf_avx512 _mm512_cos_ps
#endif
#else /* 鑷畾涔夊疄鐜?*/

/* ========== 鏍囧噯澶存枃浠?========== */
#ifdef FAST_SINCOSF_STRICT
#include <errno.h>
#include <fenv.h>
#endif

/* ========== 鍙Щ妞嶆€у畯 ========== */
#ifdef _MSC_VER
#define SINCOSF_LIKELY(x) (x)
#define SINCOSF_UNLIKELY(x) (x)
#ifdef FAST_SINCOSF_NO_FORCE_INLINE
#define SINCOSF_INLINE static inline
#else
#define SINCOSF_INLINE __forceinline
#endif
#else
#define SINCOSF_LIKELY(x) __builtin_expect(!!(x), 1)
#define SINCOSF_UNLIKELY(x) __builtin_expect(!!(x), 0)
#ifdef FAST_SINCOSF_NO_FORCE_INLINE
#define SINCOSF_INLINE static inline
#else
#define SINCOSF_INLINE static inline __attribute__((always_inline))
#endif
#endif

/* ========== 瀹夊叏妯″紡閰嶇疆 ========== */
#ifdef FAST_SINCOSF_STRICT
#define SINCOSF_ERRNO_DOMAIN() \
    do                         \
    {                          \
        errno = EDOM;          \
    } while (0)
#define SINCOSF_ERRNO_RANGE() \
    do                        \
    {                         \
        errno = ERANGE;       \
    } while (0)
#define SINCOSF_RAISE_INVALID() feraiseexcept(FE_INVALID)
#else
#define SINCOSF_ERRNO_DOMAIN() ((void)0)
#define SINCOSF_ERRNO_RANGE() ((void)0)
#define SINCOSF_RAISE_INVALID() ((void)0)
#endif

/* ========== 杩愯鏃?CPU 鐗规€ф娴?========== */
#if !defined(FAST_SINCOSF_NO_RUNTIME_DISPATCH) && defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#include <cpuid.h>
#define SINCOSF_HAVE_AVX512 (__builtin_cpu_supports("avx512f"))
#define SINCOSF_HAVE_AVX2 __builtin_cpu_supports("avx2")
#define SINCOSF_HAVE_FMA __builtin_cpu_supports("fma")
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#define SINCOSF_HAVE_NEON 1
#else
#define SINCOSF_HAVE_AVX512 0
#define SINCOSF_HAVE_AVX2 0
#define SINCOSF_HAVE_NEON 0
#define SINCOSF_HAVE_FMA 0
#endif

/* ========== 甯搁噺瀹氫箟 ========== */
static const float SINCOSF_PI = 3.141592653589793f;
static const float SINCOSF_PI_2 = 1.5707963267948966f;
static const float SINCOSF_PI_4 = 0.7853981633974483f;
static const float SINCOSF_2_PI = 0.6366197723675814f;
static const float SINCOSF_2PI = 6.283185307179586f;

/* ========== 鍙傛暟褰掔害 ========== */
SINCOSF_INLINE int sincosf_reduce(float x, float *r)
{
    float kf = x * SINCOSF_2_PI;
    int k = (int)(kf + (kf >= 0 ? 0.5f : -0.5f));
    *r = x - (float)k * SINCOSF_PI_2;
    return k & 3; /* 璞￠檺 (0-3) */
}

/* ========== 澶氶」寮忛€艰繎 ========== */
#ifdef FAST_SINCOSF_RELAXED
/* 5 闃跺椤瑰紡 */
SINCOSF_INLINE float sin_poly5(float x)
{
    const float c5 = 0.0083328241f;
    const float c3 = -0.1666665468f;
    float x2 = x * x;
    float x3 = x2 * x;
    return x + x3 * (c5 * x2 + c3);
}
SINCOSF_INLINE float cos_poly5(float x)
{
    const float c4 = 0.0416666418f;
    const float c2 = -0.4999999963f;
    const float c0 = 1.0000000000f;
    float x2 = x * x;
    return (c4 * x2 + c2) * x2 + c0;
}
#define sin_poly sin_poly5
#define cos_poly cos_poly5
#else
/* 7 闃跺椤瑰紡 */
SINCOSF_INLINE float sin_poly7(float x)
{
    const float c7 = -0.0001950727f;
    const float c5 = 0.0083320758f;
    const float c3 = -0.1666665240f;
    float x2 = x * x;
    float x3 = x2 * x;
    return x + x3 * ((c7 * x2 + c5) * x2 + c3);
}
SINCOSF_INLINE float cos_poly7(float x)
{
    const float c6 = -0.0013888395f;
    const float c4 = 0.0416666418f;
    const float c2 = -0.4999999963f;
    const float c0 = 1.0000000000f;
    float x2 = x * x;
    return ((c6 * x2 + c4) * x2 + c2) * x2 + c0;
}
#define sin_poly sin_poly7
#define cos_poly cos_poly7
#endif

/* ========== 鏍囬噺鏍稿績瀹炵幇 ========== */
SINCOSF_INLINE void fast_sincosf_scalar(float x, float *sin_out, float *cos_out)
{
    float r;
    int quadrant = sincosf_reduce(x, &r);
    float sin_r, cos_r;

    switch (quadrant)
    {
    case 0:
        sin_r = sin_poly(r);
        cos_r = cos_poly(r);
        break;
    case 1:
        sin_r = cos_poly(r);
        cos_r = -sin_poly(r);
        break;
    case 2:
        sin_r = -sin_poly(r);
        cos_r = -cos_poly(r);
        break;
    default: /* 3 */
        sin_r = -cos_poly(r);
        cos_r = sin_poly(r);
        break;
    }

    *sin_out = sin_r;
    *cos_out = cos_r;
}

SINCOSF_INLINE float fast_sinf_scalar(float x)
{
    float s, c;
    fast_sincosf_scalar(x, &s, &c);
    return s;
}

SINCOSF_INLINE float fast_cosf_scalar(float x)
{
    float s, c;
    fast_sincosf_scalar(x, &s, &c);
    return c;
}

/* ========== AVX2 鍚戦噺鍖栧疄鐜?========== */
#if defined(__AVX2__) && SINCOSF_HAVE_AVX2 && !defined(FAST_SINCOSF_NO_SIMD)
#include <immintrin.h>

/* AVX2 鍚戦噺鍖栧弬鏁板綊绾?*/
SINCOSF_INLINE void sincosf_reduce_avx2(__m256 x, __m256 *r, __m256i *quadrant)
{
    const __m256 two_over_pi = _mm256_set1_ps(SINCOSF_2_PI);
    const __m256 half = _mm256_set1_ps(0.5f);
    const __m256 pi_2 = _mm256_set1_ps(SINCOSF_PI_2);

    __m256 kf = _mm256_mul_ps(x, two_over_pi);
    kf = _mm256_add_ps(kf, half);
    __m256i k = _mm256_cvttps_epi32(kf);
    kf = _mm256_cvtepi32_ps(k);
    kf = _mm256_sub_ps(kf, half);
    k = _mm256_cvttps_epi32(kf);

    *r = _mm256_fnmadd_ps(kf, pi_2, x);
    *quadrant = _mm256_and_si256(k, _mm256_set1_epi32(3));
}

/* AVX2 鍚戦噺鍖栧椤瑰紡 */
SINCOSF_INLINE void sin_cos_poly_avx2(__m256 r, __m256 *sin_out, __m256 *cos_out)
{
    __m256 r2 = _mm256_mul_ps(r, r);
#ifdef FAST_SINCOSF_RELAXED
    /* sin 5闃?*/
    const __m256 sin_c5 = _mm256_set1_ps(0.0083328241f);
    const __m256 sin_c3 = _mm256_set1_ps(-0.1666665468f);
    __m256 r3 = _mm256_mul_ps(r2, r);
    __m256 sin_tmp = _mm256_fmadd_ps(sin_c5, r2, sin_c3);
    *sin_out = _mm256_fmadd_ps(r3, sin_tmp, r);

    /* cos 5闃?*/
    const __m256 cos_c4 = _mm256_set1_ps(0.0416666418f);
    const __m256 cos_c2 = _mm256_set1_ps(-0.4999999963f);
    const __m256 cos_c0 = _mm256_set1_ps(1.0f);
    __m256 cos_tmp = _mm256_fmadd_ps(cos_c4, r2, cos_c2);
    *cos_out = _mm256_fmadd_ps(cos_tmp, r2, cos_c0);
#else
    /* sin 7闃?*/
    const __m256 sin_c7 = _mm256_set1_ps(-0.0001950727f);
    const __m256 sin_c5 = _mm256_set1_ps(0.0083320758f);
    const __m256 sin_c3 = _mm256_set1_ps(-0.1666665240f);
    __m256 r3 = _mm256_mul_ps(r2, r);
    __m256 sin_tmp = _mm256_fmadd_ps(sin_c7, r2, sin_c5);
    sin_tmp = _mm256_fmadd_ps(sin_tmp, r2, sin_c3);
    *sin_out = _mm256_fmadd_ps(r3, sin_tmp, r);

    /* cos 7闃?*/
    const __m256 cos_c6 = _mm256_set1_ps(-0.0013888395f);
    const __m256 cos_c4 = _mm256_set1_ps(0.0416666418f);
    const __m256 cos_c2 = _mm256_set1_ps(-0.4999999963f);
    const __m256 cos_c0 = _mm256_set1_ps(1.0f);
    __m256 cos_tmp = _mm256_fmadd_ps(cos_c6, r2, cos_c4);
    cos_tmp = _mm256_fmadd_ps(cos_tmp, r2, cos_c2);
    *cos_out = _mm256_fmadd_ps(cos_tmp, r2, cos_c0);
#endif
}

SINCOSF_INLINE __m256 fast_sinf_avx2(__m256 x)
{
    __m256 r;
    __m256i quadrant;
    sincosf_reduce_avx2(x, &r, &quadrant);
    __m256 sin_r, cos_r;
    sin_cos_poly_avx2(r, &sin_r, &cos_r);

    /* 鏍规嵁璞￠檺閫夋嫨缁撴灉骞跺鐞嗙鍙?*/
    __m256i mask_q1 = _mm256_cmpeq_epi32(quadrant, _mm256_set1_epi32(1));
    __m256i mask_q2 = _mm256_cmpeq_epi32(quadrant, _mm256_set1_epi32(2));
    __m256i mask_q3 = _mm256_cmpeq_epi32(quadrant, _mm256_set1_epi32(3));
    __m256 zero = _mm256_setzero_ps();

    __m256 result = sin_r;
    result = _mm256_blendv_ps(result, cos_r, _mm256_castsi256_ps(mask_q1));
    result = _mm256_blendv_ps(result, _mm256_sub_ps(zero, sin_r), _mm256_castsi256_ps(mask_q2));
    result = _mm256_blendv_ps(result, _mm256_sub_ps(zero, cos_r), _mm256_castsi256_ps(mask_q3));
    return result;
}

SINCOSF_INLINE __m256 fast_cosf_avx2(__m256 x)
{
    __m256 r;
    __m256i quadrant;
    sincosf_reduce_avx2(x, &r, &quadrant);
    __m256 sin_r, cos_r;
    sin_cos_poly_avx2(r, &sin_r, &cos_r);

    __m256i mask_q1 = _mm256_cmpeq_epi32(quadrant, _mm256_set1_epi32(1));
    __m256i mask_q2 = _mm256_cmpeq_epi32(quadrant, _mm256_set1_epi32(2));
    __m256i mask_q3 = _mm256_cmpeq_epi32(quadrant, _mm256_set1_epi32(3));
    __m256 zero = _mm256_setzero_ps();

    __m256 result = cos_r;
    result = _mm256_blendv_ps(result, _mm256_sub_ps(zero, sin_r), _mm256_castsi256_ps(mask_q1));
    result = _mm256_blendv_ps(result, _mm256_sub_ps(zero, cos_r), _mm256_castsi256_ps(mask_q2));
    result = _mm256_blendv_ps(result, sin_r, _mm256_castsi256_ps(mask_q3));
    return result;
}

SINCOSF_INLINE void fast_sincosf_avx2(__m256 x, __m256 *sin_out, __m256 *cos_out)
{
    __m256 r;
    __m256i quadrant;
    sincosf_reduce_avx2(x, &r, &quadrant);
    __m256 sin_r, cos_r;
    sin_cos_poly_avx2(r, &sin_r, &cos_r);

    __m256i mask_q1 = _mm256_cmpeq_epi32(quadrant, _mm256_set1_epi32(1));
    __m256i mask_q2 = _mm256_cmpeq_epi32(quadrant, _mm256_set1_epi32(2));
    __m256i mask_q3 = _mm256_cmpeq_epi32(quadrant, _mm256_set1_epi32(3));
    __m256 zero = _mm256_setzero_ps();

    *sin_out = sin_r;
    *sin_out = _mm256_blendv_ps(*sin_out, cos_r, _mm256_castsi256_ps(mask_q1));
    *sin_out = _mm256_blendv_ps(*sin_out, _mm256_sub_ps(zero, sin_r), _mm256_castsi256_ps(mask_q2));
    *sin_out = _mm256_blendv_ps(*sin_out, _mm256_sub_ps(zero, cos_r), _mm256_castsi256_ps(mask_q3));

    *cos_out = cos_r;
    *cos_out = _mm256_blendv_ps(*cos_out, _mm256_sub_ps(zero, sin_r), _mm256_castsi256_ps(mask_q1));
    *cos_out = _mm256_blendv_ps(*cos_out, _mm256_sub_ps(zero, cos_r), _mm256_castsi256_ps(mask_q2));
    *cos_out = _mm256_blendv_ps(*cos_out, sin_r, _mm256_castsi256_ps(mask_q3));
}
#endif /* __AVX2__ */

/* ========== AVX-512 鍚戦噺鍖栧疄鐜?========== */
#if defined(__AVX512F__) && SINCOSF_HAVE_AVX512 && !defined(FAST_SINCOSF_NO_SIMD)
#include <immintrin.h>

SINCOSF_INLINE void sincosf_reduce_avx512(__m512 x, __m512 *r, __mmask16 *quadrant_mask[4])
{
    const __m512 two_over_pi = _mm512_set1_ps(SINCOSF_2_PI);
    const __m512 half = _mm512_set1_ps(0.5f);
    const __m512 pi_2 = _mm512_set1_ps(SINCOSF_PI_2);

    __m512 kf = _mm512_mul_ps(x, two_over_pi);
    kf = _mm512_add_ps(kf, half);
    __m512i k = _mm512_cvttps_epi32(kf);
    kf = _mm512_cvtepi32_ps(k);
    kf = _mm512_sub_ps(kf, half);
    k = _mm512_cvttps_epi32(kf);

    *r = _mm512_fnmadd_ps(kf, pi_2, x);
    __m512i quadrant = _mm512_and_si512(k, _mm512_set1_epi32(3));

    for (int q = 0; q < 4; ++q)
    {
        quadrant_mask[q] = _mm512_cmpeq_epi32_mask(quadrant, _mm512_set1_epi32(q));
    }
}

SINCOSF_INLINE void sin_cos_poly_avx512(__m512 r, __m512 *sin_out, __m512 *cos_out)
{
    __m512 r2 = _mm512_mul_ps(r, r);
#ifdef FAST_SINCOSF_RELAXED
    const __m512 sin_c5 = _mm512_set1_ps(0.0083328241f);
    const __m512 sin_c3 = _mm512_set1_ps(-0.1666665468f);
    __m512 r3 = _mm512_mul_ps(r2, r);
    __m512 sin_tmp = _mm512_fmadd_ps(sin_c5, r2, sin_c3);
    *sin_out = _mm512_fmadd_ps(r3, sin_tmp, r);

    const __m512 cos_c4 = _mm512_set1_ps(0.0416666418f);
    const __m512 cos_c2 = _mm512_set1_ps(-0.4999999963f);
    const __m512 cos_c0 = _mm512_set1_ps(1.0f);
    __m512 cos_tmp = _mm512_fmadd_ps(cos_c4, r2, cos_c2);
    *cos_out = _mm512_fmadd_ps(cos_tmp, r2, cos_c0);
#else
    const __m512 sin_c7 = _mm512_set1_ps(-0.0001950727f);
    const __m512 sin_c5 = _mm512_set1_ps(0.0083320758f);
    const __m512 sin_c3 = _mm512_set1_ps(-0.1666665240f);
    __m512 r3 = _mm512_mul_ps(r2, r);
    __m512 sin_tmp = _mm512_fmadd_ps(sin_c7, r2, sin_c5);
    sin_tmp = _mm512_fmadd_ps(sin_tmp, r2, sin_c3);
    *sin_out = _mm512_fmadd_ps(r3, sin_tmp, r);

    const __m512 cos_c6 = _mm512_set1_ps(-0.0013888395f);
    const __m512 cos_c4 = _mm512_set1_ps(0.0416666418f);
    const __m512 cos_c2 = _mm512_set1_ps(-0.4999999963f);
    const __m512 cos_c0 = _mm512_set1_ps(1.0f);
    __m512 cos_tmp = _mm512_fmadd_ps(cos_c6, r2, cos_c4);
    cos_tmp = _mm512_fmadd_ps(cos_tmp, r2, cos_c2);
    *cos_out = _mm512_fmadd_ps(cos_tmp, r2, cos_c0);
#endif
}

SINCOSF_INLINE __m512 fast_sinf_avx512(__m512 x)
{
    __m512 r;
    __mmask16 mask_q[4];
    sincosf_reduce_avx512(x, &r, mask_q);
    __m512 sin_r, cos_r;
    sin_cos_poly_avx512(r, &sin_r, &cos_r);

    __m512 zero = _mm512_setzero_ps();
    __m512 result = sin_r;
    result = _mm512_mask_blend_ps(mask_q[1], result, cos_r);
    result = _mm512_mask_blend_ps(mask_q[2], result, _mm512_sub_ps(zero, sin_r));
    result = _mm512_mask_blend_ps(mask_q[3], result, _mm512_sub_ps(zero, cos_r));
    return result;
}

SINCOSF_INLINE __m512 fast_cosf_avx512(__m512 x)
{
    __m512 r;
    __mmask16 mask_q[4];
    sincosf_reduce_avx512(x, &r, mask_q);
    __m512 sin_r, cos_r;
    sin_cos_poly_avx512(r, &sin_r, &cos_r);

    __m512 zero = _mm512_setzero_ps();
    __m512 result = cos_r;
    result = _mm512_mask_blend_ps(mask_q[1], result, _mm512_sub_ps(zero, sin_r));
    result = _mm512_mask_blend_ps(mask_q[2], result, _mm512_sub_ps(zero, cos_r));
    result = _mm512_mask_blend_ps(mask_q[3], result, sin_r);
    return result;
}

SINCOSF_INLINE void fast_sincosf_avx512(__m512 x, __m512 *sin_out, __m512 *cos_out)
{
    __m512 r;
    __mmask16 mask_q[4];
    sincosf_reduce_avx512(x, &r, mask_q);
    __m512 sin_r, cos_r;
    sin_cos_poly_avx512(r, &sin_r, &cos_r);

    __m512 zero = _mm512_setzero_ps();
    *sin_out = sin_r;
    *sin_out = _mm512_mask_blend_ps(mask_q[1], *sin_out, cos_r);
    *sin_out = _mm512_mask_blend_ps(mask_q[2], *sin_out, _mm512_sub_ps(zero, sin_r));
    *sin_out = _mm512_mask_blend_ps(mask_q[3], *sin_out, _mm512_sub_ps(zero, cos_r));

    *cos_out = cos_r;
    *cos_out = _mm512_mask_blend_ps(mask_q[1], *cos_out, _mm512_sub_ps(zero, sin_r));
    *cos_out = _mm512_mask_blend_ps(mask_q[2], *cos_out, _mm512_sub_ps(zero, cos_r));
    *cos_out = _mm512_mask_blend_ps(mask_q[3], *cos_out, sin_r);
}
#endif /* __AVX512F__ */

/* ========== ARM NEON 鍚戦噺鍖栧疄鐜?========== */
#if defined(__ARM_NEON) && SINCOSF_HAVE_NEON && !defined(FAST_SINCOSF_NO_SIMD)
#include <arm_neon.h>

SINCOSF_INLINE void sincosf_reduce_neon(float32x4_t x, float32x4_t *r, uint32x4_t *quadrant)
{
    const float32x4_t two_over_pi = vdupq_n_f32(SINCOSF_2_PI);
    const float32x4_t half = vdupq_n_f32(0.5f);
    const float32x4_t pi_2 = vdupq_n_f32(SINCOSF_PI_2);

    float32x4_t kf = vmulq_f32(x, two_over_pi);
    kf = vaddq_f32(kf, half);
    int32x4_t k = vcvtq_s32_f32(kf);
    kf = vcvtq_f32_s32(k);
    kf = vsubq_f32(kf, half);
    k = vcvtq_s32_f32(kf);

    *r = vmlsq_f32(x, kf, pi_2);
    *quadrant = vandq_u32(vreinterpretq_u32_s32(k), vdupq_n_u32(3));
}

SINCOSF_INLINE void sin_cos_poly_neon(float32x4_t r, float32x4_t *sin_out, float32x4_t *cos_out)
{
    float32x4_t r2 = vmulq_f32(r, r);
#ifdef FAST_SINCOSF_RELAXED
    const float32x4_t sin_c5 = vdupq_n_f32(0.0083328241f);
    const float32x4_t sin_c3 = vdupq_n_f32(-0.1666665468f);
    float32x4_t r3 = vmulq_f32(r2, r);
    float32x4_t sin_tmp = vmlaq_f32(sin_c3, sin_c5, r2);
    *sin_out = vmlaq_f32(r, r3, sin_tmp);

    const float32x4_t cos_c4 = vdupq_n_f32(0.0416666418f);
    const float32x4_t cos_c2 = vdupq_n_f32(-0.4999999963f);
    const float32x4_t cos_c0 = vdupq_n_f32(1.0f);
    float32x4_t cos_tmp = vmlaq_f32(cos_c2, cos_c4, r2);
    *cos_out = vmlaq_f32(cos_c0, cos_tmp, r2);
#else
    const float32x4_t sin_c7 = vdupq_n_f32(-0.0001950727f);
    const float32x4_t sin_c5 = vdupq_n_f32(0.0083320758f);
    const float32x4_t sin_c3 = vdupq_n_f32(-0.1666665240f);
    float32x4_t r3 = vmulq_f32(r2, r);
    float32x4_t sin_tmp = vmlaq_f32(sin_c5, sin_c7, r2);
    sin_tmp = vmlaq_f32(sin_c3, sin_tmp, r2);
    *sin_out = vmlaq_f32(r, r3, sin_tmp);

    const float32x4_t cos_c6 = vdupq_n_f32(-0.0013888395f);
    const float32x4_t cos_c4 = vdupq_n_f32(0.0416666418f);
    const float32x4_t cos_c2 = vdupq_n_f32(-0.4999999963f);
    const float32x4_t cos_c0 = vdupq_n_f32(1.0f);
    float32x4_t cos_tmp = vmlaq_f32(cos_c4, cos_c6, r2);
    cos_tmp = vmlaq_f32(cos_c2, cos_tmp, r2);
    *cos_out = vmlaq_f32(cos_c0, cos_tmp, r2);
#endif
}

SINCOSF_INLINE float32x4_t fast_sinf_neon(float32x4_t x)
{
    float32x4_t r;
    uint32x4_t quadrant;
    sincosf_reduce_neon(x, &r, &quadrant);
    float32x4_t sin_r, cos_r;
    sin_cos_poly_neon(r, &sin_r, &cos_r);

    uint32x4_t mask_q1 = vceqq_u32(quadrant, vdupq_n_u32(1));
    uint32x4_t mask_q2 = vceqq_u32(quadrant, vdupq_n_u32(2));
    uint32x4_t mask_q3 = vceqq_u32(quadrant, vdupq_n_u32(3));
    float32x4_t zero = vdupq_n_f32(0.0f);

    float32x4_t result = sin_r;
    result = vbslq_f32(mask_q1, cos_r, result);
    result = vbslq_f32(mask_q2, vsubq_f32(zero, sin_r), result);
    result = vbslq_f32(mask_q3, vsubq_f32(zero, cos_r), result);
    return result;
}

SINCOSF_INLINE float32x4_t fast_cosf_neon(float32x4_t x)
{
    float32x4_t r;
    uint32x4_t quadrant;
    sincosf_reduce_neon(x, &r, &quadrant);
    float32x4_t sin_r, cos_r;
    sin_cos_poly_neon(r, &sin_r, &cos_r);

    uint32x4_t mask_q1 = vceqq_u32(quadrant, vdupq_n_u32(1));
    uint32x4_t mask_q2 = vceqq_u32(quadrant, vdupq_n_u32(2));
    uint32x4_t mask_q3 = vceqq_u32(quadrant, vdupq_n_u32(3));
    float32x4_t zero = vdupq_n_f32(0.0f);

    float32x4_t result = cos_r;
    result = vbslq_f32(mask_q1, vsubq_f32(zero, sin_r), result);
    result = vbslq_f32(mask_q2, vsubq_f32(zero, cos_r), result);
    result = vbslq_f32(mask_q3, sin_r, result);
    return result;
}

SINCOSF_INLINE void fast_sincosf_neon(float32x4_t x, float32x4_t *sin_out, float32x4_t *cos_out)
{
    float32x4_t r;
    uint32x4_t quadrant;
    sincosf_reduce_neon(x, &r, &quadrant);
    float32x4_t sin_r, cos_r;
    sin_cos_poly_neon(r, &sin_r, &cos_r);

    uint32x4_t mask_q1 = vceqq_u32(quadrant, vdupq_n_u32(1));
    uint32x4_t mask_q2 = vceqq_u32(quadrant, vdupq_n_u32(2));
    uint32x4_t mask_q3 = vceqq_u32(quadrant, vdupq_n_u32(3));
    float32x4_t zero = vdupq_n_f32(0.0f);

    *sin_out = sin_r;
    *sin_out = vbslq_f32(mask_q1, cos_r, *sin_out);
    *sin_out = vbslq_f32(mask_q2, vsubq_f32(zero, sin_r), *sin_out);
    *sin_out = vbslq_f32(mask_q3, vsubq_f32(zero, cos_r), *sin_out);

    *cos_out = cos_r;
    *cos_out = vbslq_f32(mask_q1, vsubq_f32(zero, sin_r), *cos_out);
    *cos_out = vbslq_f32(mask_q2, vsubq_f32(zero, cos_r), *cos_out);
    *cos_out = vbslq_f32(mask_q3, sin_r, *cos_out);
}
#endif /* __ARM_NEON */

/* ========== 杩愯鏃?CPU 鍒嗗彂 ========== */
#if !defined(FAST_SINCOSF_NO_RUNTIME_DISPATCH) && (defined(__x86_64__) || defined(__i386__))
SINCOSF_INLINE void fast_sincosf(float x, float *s, float *c)
{
    if (SINCOSF_HAVE_AVX512)
    {
        __m128 vx = _mm_set_ss(x);
        __m512 vx512 = _mm512_broadcastss_ps(vx);
        __m512 vs, vc;
        fast_sincosf_avx512(vx512, &vs, &vc);
        *s = _mm_cvtss_f32(_mm512_castps512_ps128(vs));
        *c = _mm_cvtss_f32(_mm512_castps512_ps128(vc));
    }
    else if (SINCOSF_HAVE_AVX2)
    {
        __m128 vx = _mm_set_ss(x);
        __m256 vx256 = _mm256_broadcastss_ps(vx);
        __m256 vs, vc;
        fast_sincosf_avx2(vx256, &vs, &vc);
        *s = _mm_cvtss_f32(_mm256_castps256_ps128(vs));
        *c = _mm_cvtss_f32(_mm256_castps256_ps128(vc));
    }
    else
    {
        fast_sincosf_scalar(x, s, c);
    }
}

SINCOSF_INLINE float fast_sinf(float x)
{
    if (SINCOSF_HAVE_AVX512)
    {
        __m128 vx = _mm_set_ss(x);
        __m512 vx512 = _mm512_broadcastss_ps(vx);
        __m512 vy = fast_sinf_avx512(vx512);
        return _mm_cvtss_f32(_mm512_castps512_ps128(vy));
    }
    else if (SINCOSF_HAVE_AVX2)
    {
        __m128 vx = _mm_set_ss(x);
        __m256 vx256 = _mm256_broadcastss_ps(vx);
        __m256 vy = fast_sinf_avx2(vx256);
        return _mm_cvtss_f32(_mm256_castps256_ps128(vy));
    }
    else
    {
        return fast_sinf_scalar(x);
    }
}

SINCOSF_INLINE float fast_cosf(float x)
{
    if (SINCOSF_HAVE_AVX512)
    {
        __m128 vx = _mm_set_ss(x);
        __m512 vx512 = _mm512_broadcastss_ps(vx);
        __m512 vy = fast_cosf_avx512(vx512);
        return _mm_cvtss_f32(_mm512_castps512_ps128(vy));
    }
    else if (SINCOSF_HAVE_AVX2)
    {
        __m128 vx = _mm_set_ss(x);
        __m256 vx256 = _mm256_broadcastss_ps(vx);
        __m256 vy = fast_cosf_avx2(vx256);
        return _mm_cvtss_f32(_mm256_castps256_ps128(vy));
    }
    else
    {
        return fast_cosf_scalar(x);
    }
}
#elif defined(__ARM_NEON) && SINCOSF_HAVE_NEON
SINCOSF_INLINE void fast_sincosf(float x, float *s, float *c)
{
    float32x4_t vx = vdupq_n_f32(x);
    float32x4_t vs, vc;
    fast_sincosf_neon(vx, &vs, &vc);
    *s = vgetq_lane_f32(vs, 0);
    *c = vgetq_lane_f32(vc, 0);
}
SINCOSF_INLINE float fast_sinf(float x)
{
    float32x4_t vx = vdupq_n_f32(x);
    float32x4_t vy = fast_sinf_neon(vx);
    return vgetq_lane_f32(vy, 0);
}
SINCOSF_INLINE float fast_cosf(float x)
{
    float32x4_t vx = vdupq_n_f32(x);
    float32x4_t vy = fast_cosf_neon(vx);
    return vgetq_lane_f32(vy, 0);
}
#else
SINCOSF_INLINE void fast_sincosf(float x, float *s, float *c)
{
    fast_sincosf_scalar(x, s, c);
}
SINCOSF_INLINE float fast_sinf(float x)
{
    return fast_sinf_scalar(x);
}
SINCOSF_INLINE float fast_cosf(float x)
{
    return fast_cosf_scalar(x);
}
#endif

#endif /* FAST_SINCOSF_USE_STDLIB */
#endif /* FAST_SINCOSF_H */

/* 恢复 GCC 诊断 */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#ifdef __cplusplus
}
#endif

#endif /* FAST_MATH_FULL_H */