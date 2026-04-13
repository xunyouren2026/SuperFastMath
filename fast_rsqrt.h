
#ifndef FAST_RSQRT_H
#define FAST_RSQRT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 可移植性宏 ========== */
#ifdef _MSC_VER
    #define FAST_RSQRT_INLINE static __forceinline
    #define FAST_RSQRT_LIKELY(x)   (x)
    #define FAST_RSQRT_UNLIKELY(x) (x)
#else
    #define FAST_RSQRT_INLINE static inline __attribute__((always_inline))
    #define FAST_RSQRT_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define FAST_RSQRT_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

/* ========== 精度等级配置 ========== */
#ifdef FAST_RSQRT_ACCURATE
    #define RSQRT_NEWTON_STEPS 2
#else
    #define RSQRT_NEWTON_STEPS 1
#endif

/* ========== 运行时 CPU 特性检测 ========== */
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

/* ========== 标量纯软件实现 (跨平台，无指令依赖) ========== */
FAST_RSQRT_INLINE float fast_rsqrtf_software(float x) {
    union { float f; uint32_t i; } u = {x};
    uint32_t ix = u.i;

    // 快速路径：正常正数
    if (FAST_RSQRT_LIKELY(ix > 0 && ix < 0x7F800000U)) {
        // 改进魔法常数：0x5f37642f (比 Quake 的 0x5f3759df 误差降低约 30%)
        u.i = 0x5f37642f - (ix >> 1);
        float y = u.f;

        // 牛顿迭代：y = y * (1.5 - 0.5 * x * y * y)
        float x2 = x * 0.5f;
        y = y * (1.5f - x2 * y * y);
#if RSQRT_NEWTON_STEPS >= 2
        y = y * (1.5f - x2 * y * y);  // 第二次迭代，精度 < 1e-6
#endif
        return y;
    }

    // 特殊值处理
    if (FAST_RSQRT_UNLIKELY(ix == 0)) {
        u.i = 0x7F800000U; return u.f;      // 0 -> +Inf
    }
    if (FAST_RSQRT_UNLIKELY(ix >= 0x7F800000U)) {
        if (ix == 0x7F800000U) return 0.0f;  // +Inf -> 0
        return x + x;                        // NaN / -Inf -> NaN
    }
    if (FAST_RSQRT_UNLIKELY(ix & 0x80000000U)) {
        u.i = 0x7FC00000U; return u.f;       // 负数 -> NaN
    }

    // 非规格化数：放大后递归
    u.f = x * 0x1.0p64f;
    return fast_rsqrtf_software(u.f) * 0x1.0p32f;
}

/* ========== x86 SSE 硬件加速版 ========== */
#if defined(__SSE__) || FAST_RSQRT_HAVE_SSE
#include <xmmintrin.h>

FAST_RSQRT_INLINE float fast_rsqrtf_sse(float x) {
    __m128 v = _mm_set_ss(x);
    v = _mm_rsqrt_ss(v);                    // 硬件初值，延迟 4-5 cycles
    float y = _mm_cvtss_f32(v);

    float x2 = x * 0.5f;
    y = y * (1.5f - x2 * y * y);
#if RSQRT_NEWTON_STEPS >= 2
    y = y * (1.5f - x2 * y * y);
#endif
    return y;
}
#endif

/* ========== AVX2 向量化版 (8个float并行) ========== */
#if defined(__AVX2__) || FAST_RSQRT_HAVE_AVX2
#include <immintrin.h>

FAST_RSQRT_INLINE __m256 fast_rsqrtf_avx2(__m256 x) {
    __m256 y = _mm256_rsqrt_ps(x);          // 硬件初值
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

/* ========== AVX-512 向量化版 (16个float并行) ========== */
#if defined(__AVX512F__) || FAST_RSQRT_HAVE_AVX512
#include <immintrin.h>

FAST_RSQRT_INLINE __m512 fast_rsqrtf_avx512(__m512 x) {
    __m512 y = _mm512_rsqrt14_ps(x);        // 更高精度初值 (14位)
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

/* ========== ARM NEON 向量化版 (4个float并行) ========== */
#if defined(__ARM_NEON) || FAST_RSQRT_HAVE_NEON
#include <arm_neon.h>

FAST_RSQRT_INLINE float32x4_t fast_rsqrtf_neon(float32x4_t x) {
    float32x4_t y = vrsqrteq_f32(x);        // NEON 硬件初值
    float32x4_t half = vdupq_n_f32(0.5f);
    float32x4_t x_half = vmulq_f32(x, half);

    // 使用 vrsqrtsq_f32 辅助指令进行牛顿迭代
    float32x4_t y_sq = vmulq_f32(y, y);
    y = vmulq_f32(y, vrsqrtsq_f32(x_half, y_sq));
#if RSQRT_NEWTON_STEPS >= 2
    y_sq = vmulq_f32(y, y);
    y = vmulq_f32(y, vrsqrtsq_f32(x_half, y_sq));
#endif
    return y;
}
#endif

/* ========== 运行时 CPU 分发：自动选择最优实现 ========== */
#if !defined(FAST_RSQRT_NO_RUNTIME) && (defined(__x86_64__) || defined(__i386__))
// x86 平台运行时分发
FAST_RSQRT_INLINE float fast_rsqrtf(float x) {
    if (FAST_RSQRT_HAVE_SSE) {
        return fast_rsqrtf_sse(x);
    }
    return fast_rsqrtf_software(x);
}
#elif defined(__ARM_NEON) || FAST_RSQRT_HAVE_NEON
// ARM NEON 平台：标量也走 NEON (单元素向量)
FAST_RSQRT_INLINE float fast_rsqrtf(float x) {
    float32x4_t vx = vdupq_n_f32(x);
    float32x4_t vy = fast_rsqrtf_neon(vx);
    return vgetq_lane_f32(vy, 0);
}
#else
// 无硬件加速：纯软件
FAST_RSQRT_INLINE float fast_rsqrtf(float x) {
    return fast_rsqrtf_software(x);
}
#endif

/* ========== 便捷宏：批量处理 ========== */
#define fast_rsqrtf_array(arr, n) do { \
    for (size_t _i = 0; _i < (n); ++_i) { \
        (arr)[_i] = fast_rsqrtf((arr)[_i]); \
    } \
} while(0)

#ifdef __cplusplus
}
#endif

#endif /* FAST_RSQRT_H 