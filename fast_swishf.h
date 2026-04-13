

#ifndef FAST_SWISHF_H
#define FAST_SWISHF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ========== 平台检测与降级 ========== */
#if defined(FAST_SWISHF_FALLBACK_STDLIB)
#define FAST_SWISHF_USE_STDLIB 1
#else
#define FAST_SWISHF_USE_STDLIB 0
#endif

#if FAST_SWISHF_USE_STDLIB
#include <math.h>
#define fast_swishf(x) ((x) / (1.0f + expf(-(x))))
#else

/* ========== 可移植性宏 ========== */
#ifdef _MSC_VER
#define SWISHF_LIKELY(x) (x)
#define SWISHF_UNLIKELY(x) (x)
#define SWISHF_INLINE __forceinline
#else
#define SWISHF_LIKELY(x) __builtin_expect(!!(x), 1)
#define SWISHF_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define SWISHF_INLINE static inline __attribute__((always_inline))
#endif

/* ========== 运行时 CPU 特性检测 ========== */
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

/* 依赖 fast_sigmoidf.h */
#ifndef FAST_SIGMOIDF_H
#include "fast_sigmoidf.h"
#endif

/* ========== 标量核心实现 ========== */
SWISHF_INLINE float fast_swishf_scalar(float x)
{
    return x * fast_sigmoidf(x);
}

/* ========== 向量化版本 ========== */
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

/* ========== 运行时 CPU 分发 ========== */
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