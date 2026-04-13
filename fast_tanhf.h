

#ifndef FAST_TANHF_H
#define FAST_TANHF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ========== 平台检测与降级 ========== */
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
#else /* 自定义实现 */

/* ========== 标准头文件 ========== */
#ifdef FAST_TANHF_STRICT
#include <errno.h>
#include <fenv.h>
#endif

/* ========== 可移植性宏 ========== */
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

/* ========== 安全模式配置 ========== */
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

/* 忽略类型双关警告 */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

/* ========== 运行时 CPU 特性检测 ========== */
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

/* ========== 依赖项处理 ========== */
#ifndef FAST_TANHF_NO_EXPF
/* 使用 fast_expf 作为指数引擎，需要 fast_expf 定义 */
#ifndef FAST_EXPF_H
#include "fast_expf.h" /* 假定存在，或内嵌定义 */
#endif
#endif

/* ========== 内置多项式逼近（无 expf 依赖时使用） ========== */
#ifdef FAST_TANHF_RELAXED
/* 5 阶多项式，误差 ~1e-6 */
TANHF_INLINE float tanh_poly5(float x)
{
    float x2 = x * x;
    /* 系数 (tanh(x)/x 的极小极大多项式) */
    const float c0 = 1.0f;
    const float c1 = -0.333333333f;
    const float c2 = 0.133333333f;
    const float c3 = -0.053968254f;
    const float c4 = 0.021869489f;
    return x * (c0 + x2 * (c1 + x2 * (c2 + x2 * (c3 + x2 * c4))));
}
#else
/* 7 阶多项式，误差 < 1e-7 */
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

/* ========== 标量核心实现 ========== */
TANHF_INLINE float fast_tanhf_scalar(float x)
{
    /* 处理符号：tanh 是奇函数 */
    union {
        float f;
        uint32_t i;
    } u = {x};
    uint32_t sign = u.i & 0x80000000U;
    float ax = fabsf(x);

    /* 小输入直接返回 x */
    if (ax < 0.000244140625f)
    { /* 2^{-12} */
        return x;
    }

    /* 大输入饱和为 ±1 */
    if (ax > 19.0f)
    {
        u.i = sign | 0x3F800000U; /* 1.0f 带符号 */
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

/* ========== 向量化版本 ========== */

/* --- AVX2 单精度 --- */
#if defined(__AVX2__) && TANHF_HAVE_AVX2 && !defined(FAST_TANHF_NO_SIMD)
#include <immintrin.h>
#ifndef FAST_TANHF_NO_EXPF
/* 需要 fast_expf_avx2 声明 */
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
    /* 使用多项式逼近（向量化） */
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

/* --- AVX-512 单精度 --- */
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

/* --- ARM NEON 单精度 --- */
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

/* ========== 运行时 CPU 分发 ========== */
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

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif /* FAST_TANHF_USE_STDLIB */
#endif /* FAST_TANHF_H */