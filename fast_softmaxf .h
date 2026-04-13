
#ifndef FAST_SOFTMAXF_H
#define FAST_SOFTMAXF_H

#include <stdint.h>
#include <math.h>  /* for fmaxf, expf */

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 平台检测 ========== */
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

/* ========== 可移植性宏 ========== */
#ifdef _MSC_VER
    #define SOFTMAX_INLINE static __forceinline
#else
    #define SOFTMAX_INLINE static inline __attribute__((always_inline))
#endif

/* 指数函数选择 */
#ifdef FAST_SOFTMAX_USE_EXPF
    #ifndef FAST_EXPF_H
        #include "fast_expf.h"
    #endif
    #define SOFTMAX_EXP fast_expf
#else
    #define SOFTMAX_EXP expf
#endif

/* ========== 标量实现 ========== */
SOFTMAX_INLINE void fast_softmaxf_scalar(float* x, int n) {
    /* 找出最大值以防止指数溢出 */
    float max_val = x[0];
    for (int i = 1; i < n; ++i) {
        if (x[i] > max_val) max_val = x[i];
    }

    /* 计算 exp(x[i] - max) 并求和 */
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) {
        x[i] = SOFTMAX_EXP(x[i] - max_val);
        sum += x[i];
    }

    /* 归一化 */
    float inv_sum = 1.0f / sum;
    for (int i = 0; i < n; ++i) {
        x[i] *= inv_sum;
    }
}

/* ========== AVX2 向量化实现 ========== */
#if defined(__AVX2__) && SOFTMAX_HAVE_AVX2 && !defined(FAST_SOFTMAX_NO_SIMD)
#include <immintrin.h>
#ifdef FAST_SOFTMAX_USE_EXPF
#ifndef FAST_EXPF_AVX2_DECLARED
#define FAST_EXPF_AVX2_DECLARED
extern __m256 fast_expf_avx2(__m256 x);
#endif
#define SOFTMAX_EXP_VEC fast_expf_avx2
#else
/* 如果标准库没有提供 _mm256_exp_ps，回退到标量循环 */
#define SOFTMAX_EXP_VEC _mm256_exp_ps
#ifndef _mm256_exp_ps
#define SOFTMAX_FALLBACK_SCALAR_EXP
#endif
#endif

SOFTMAX_INLINE void fast_softmaxf_avx2(float* x, int n) {
    /* 找出最大值（标量或向量化归约） */
    float max_val = x[0];
    int i = 0;
    for (; i + 7 < n; i += 8) {
        __m256 v = _mm256_loadu_ps(&x[i]);
        __m256 maxv = v;
        /* 简单标量比较保持代码清晰，实际可用 _mm256_max_ps 进行向量归约 */
        float tmp[8];
        _mm256_storeu_ps(tmp, v);
        for (int k = 0; k < 8; ++k) if (tmp[k] > max_val) max_val = tmp[k];
    }
    for (; i < n; ++i) {
        if (x[i] > max_val) max_val = x[i];
    }

    __m256 max_vec = _mm256_set1_ps(max_val);
    __m256 sum_vec = _mm256_setzero_ps();

    /* 主循环：计算 exp 并累加 */
    i = 0;
    for (; i + 7 < n; i += 8) {
        __m256 v = _mm256_loadu_ps(&x[i]);
        v = _mm256_sub_ps(v, max_vec);
#ifdef SOFTMAX_FALLBACK_SCALAR_EXP
        /* 回退到标量 exp */
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

    /* 水平求和 sum_vec */
    float sum = 0.0f;
    float tmp[8];
    _mm256_storeu_ps(tmp, sum_vec);
    for (int k = 0; k < 8; ++k) sum += tmp[k];

    /* 处理剩余元素 */
    for (; i < n; ++i) {
        x[i] = SOFTMAX_EXP(x[i] - max_val);
        sum += x[i];
    }

    /* 归一化 */
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

/* ========== AVX-512 向量化实现 ========== */
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
    /* 找出最大值 */
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

    /* 水平求和 sum_vec */
    float sum = _mm512_reduce_add_ps(sum_vec);

    /* 处理剩余元素 */
    for (; i < n; ++i) {
        x[i] = SOFTMAX_EXP(x[i] - max_val);
        sum += x[i];
    }

    /* 归一化 */
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

/* ========== 运行时 CPU 分发 ========== */
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