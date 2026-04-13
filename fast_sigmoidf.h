

#ifndef FAST_SIGMOIDF_H
#define FAST_SIGMOIDF_H

#include <stdint.h>
#include <math.h> /* for fabsf */

#ifdef __cplusplus
extern "C"
{
#endif

/* ========== 平台检测与降级 ========== */
#if defined(FAST_SIGMOIDF_FALLBACK_STDLIB)
#define FAST_SIGMOIDF_USE_STDLIB 1
#else
#define FAST_SIGMOIDF_USE_STDLIB 0
#endif

#if FAST_SIGMOIDF_USE_STDLIB
#include <math.h>
#define fast_sigmoidf(x) (1.0f / (1.0f + expf(-(x))))
#else

/* ========== 可移植性宏 ========== */
#ifdef _MSC_VER
#define SIGMOIDF_LIKELY(x) (x)
#define SIGMOIDF_UNLIKELY(x) (x)
#define SIGMOIDF_INLINE __forceinline
#else
#define SIGMOIDF_LIKELY(x) __builtin_expect(!!(x), 1)
#define SIGMOIDF_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define SIGMOIDF_INLINE static inline __attribute__((always_inline))
#endif

/* ========== 运行时 CPU 特性检测 ========== */
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

/* 依赖 fast_expf.h */
#ifndef FAST_EXPF_H
#include "fast_expf.h"
#endif

/* ========== 标量核心实现 ========== */
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

/* ========== 向量化版本 ========== */

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

/* ========== 运行时 CPU 分发 ========== */
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