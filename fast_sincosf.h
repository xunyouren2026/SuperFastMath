
#ifndef FAST_SINCOSF_H
#define FAST_SINCOSF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ========== 平台检测与降级 ========== */
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
#else /* 自定义实现 */

/* ========== 标准头文件 ========== */
#ifdef FAST_SINCOSF_STRICT
#include <errno.h>
#include <fenv.h>
#endif

/* ========== 可移植性宏 ========== */
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

/* ========== 安全模式配置 ========== */
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

/* 忽略类型双关警告 */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

/* ========== 运行时 CPU 特性检测 ========== */
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

/* ========== 常量定义 ========== */
static const float SINCOSF_PI = 3.141592653589793f;
static const float SINCOSF_PI_2 = 1.5707963267948966f;
static const float SINCOSF_PI_4 = 0.7853981633974483f;
static const float SINCOSF_2_PI = 0.6366197723675814f;
static const float SINCOSF_2PI = 6.283185307179586f;

/* ========== 参数归约 ========== */
SINCOSF_INLINE int sincosf_reduce(float x, float *r)
{
    float kf = x * SINCOSF_2_PI;
    int k = (int)(kf + (kf >= 0 ? 0.5f : -0.5f));
    *r = x - (float)k * SINCOSF_PI_2;
    return k & 3; /* 象限 (0-3) */
}

/* ========== 多项式逼近 ========== */
#ifdef FAST_SINCOSF_RELAXED
/* 5 阶多项式 */
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
/* 7 阶多项式 */
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

/* ========== 标量核心实现 ========== */
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

/* ========== AVX2 向量化实现 ========== */
#if defined(__AVX2__) && SINCOSF_HAVE_AVX2 && !defined(FAST_SINCOSF_NO_SIMD)
#include <immintrin.h>

/* AVX2 向量化参数归约 */
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

/* AVX2 向量化多项式 */
SINCOSF_INLINE void sin_cos_poly_avx2(__m256 r, __m256 *sin_out, __m256 *cos_out)
{
    __m256 r2 = _mm256_mul_ps(r, r);
#ifdef FAST_SINCOSF_RELAXED
    /* sin 5阶 */
    const __m256 sin_c5 = _mm256_set1_ps(0.0083328241f);
    const __m256 sin_c3 = _mm256_set1_ps(-0.1666665468f);
    __m256 r3 = _mm256_mul_ps(r2, r);
    __m256 sin_tmp = _mm256_fmadd_ps(sin_c5, r2, sin_c3);
    *sin_out = _mm256_fmadd_ps(r3, sin_tmp, r);

    /* cos 5阶 */
    const __m256 cos_c4 = _mm256_set1_ps(0.0416666418f);
    const __m256 cos_c2 = _mm256_set1_ps(-0.4999999963f);
    const __m256 cos_c0 = _mm256_set1_ps(1.0f);
    __m256 cos_tmp = _mm256_fmadd_ps(cos_c4, r2, cos_c2);
    *cos_out = _mm256_fmadd_ps(cos_tmp, r2, cos_c0);
#else
    /* sin 7阶 */
    const __m256 sin_c7 = _mm256_set1_ps(-0.0001950727f);
    const __m256 sin_c5 = _mm256_set1_ps(0.0083320758f);
    const __m256 sin_c3 = _mm256_set1_ps(-0.1666665240f);
    __m256 r3 = _mm256_mul_ps(r2, r);
    __m256 sin_tmp = _mm256_fmadd_ps(sin_c7, r2, sin_c5);
    sin_tmp = _mm256_fmadd_ps(sin_tmp, r2, sin_c3);
    *sin_out = _mm256_fmadd_ps(r3, sin_tmp, r);

    /* cos 7阶 */
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

    /* 根据象限选择结果并处理符号 */
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

/* ========== AVX-512 向量化实现 ========== */
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

/* ========== ARM NEON 向量化实现 ========== */
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

/* ========== 运行时 CPU 分发 ========== */
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

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif /* FAST_SINCOSF_USE_STDLIB */
#endif /* FAST_SINCOSF_H */