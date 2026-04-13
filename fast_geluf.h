

#ifndef FAST_GELUF_H
#define FAST_GELUF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ========== 平台检测与降级 ========== */
#if defined(FAST_GELUF_FALLBACK_STDLIB)
#define FAST_GELUF_USE_STDLIB 1
#else
#define FAST_GELUF_USE_STDLIB 0
#endif

#if FAST_GELUF_USE_STDLIB
#include <math.h>
#define fast_geluf(x) (0.5f * (x) * (1.0f + tanhf(0.7978845608028654f * ((x) + 0.044715f * (x) * (x) * (x)))))
#else

/* ========== 可移植性宏 ========== */
#ifdef _MSC_VER
#define GELUF_LIKELY(x) (x)
#define GELUF_UNLIKELY(x) (x)
#define GELUF_INLINE __forceinline
#else
#define GELUF_LIKELY(x) __builtin_expect(!!(x), 1)
#define GELUF_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define GELUF_INLINE static inline __attribute__((always_inline))
#endif

/* ========== 运行时 CPU 特性检测 ========== */
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

/* 依赖 fast_tanhf.h */
#ifndef FAST_TANHF_H
#include "fast_tanhf.h"
#endif

/* ========== 常量 ========== */
static const float GELUF_SQRT_2_OVER_PI = 0.7978845608028654f;
static const float GELUF_COEFF = 0.044715f;

/* ========== 标量核心实现 ========== */
GELUF_INLINE float fast_geluf_scalar(float x)
{
    float x2 = x * x;
    float x3 = x2 * x;
    float inner = GELUF_SQRT_2_OVER_PI * (x + GELUF_COEFF * x3);
    float tanh_inner = fast_tanhf(inner);
    return 0.5f * x * (1.0f + tanh_inner);
}

/* ========== 向量化版本 ========== */
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

/* ========== 运行时 CPU 分发 ========== */
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