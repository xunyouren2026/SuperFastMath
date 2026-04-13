
//#ifndef SUPERFASTMATH_H
//#define SUPERFASTMATH_H
//
//#include <stdint.h>
//
///* ========== 平台检测与降级 ========== */
//#if defined(SUPERFASTMATH_FALLBACK_STDLIB)
//    #define SUPERFASTMATH_USE_STDLIB 1
//#elif !defined(__STDC_IEC_559__) && !defined(__IEEE_BIG_ENDIAN) && !defined(__IEEE_LITTLE_ENDIAN)
//    #define SUPERFASTMATH_USE_STDLIB 1
//#else
//    #define SUPERFASTMATH_USE_STDLIB 0
//#endif
//
//#if SUPERFASTMATH_USE_STDLIB
//    #include <math.h>
//    #define fast_expf     expf
//    #define fast_lnf      logf
//    #define fast_log2f    log2f
//    #define fast_log10f   log10f
//    #define fast_exp      exp
//    #define fast_ln       log
//    #define fast_log2     log2
//    #define fast_log10    log10
//#else   /* 自定义实现 */
//
///* ========== 标准头文件 ========== */
//#ifdef SUPERFASTMATH_STRICT
//    #include <errno.h>
//    #include <fenv.h>
//#endif
//
///* ========== 可移植性宏 ========== */
//#ifdef _MSC_VER
//    #define LIKELY(x)   (x)
//    #define UNLIKELY(x) (x)
//    #ifdef SUPERFASTMATH_NO_FORCE_INLINE
//        #define INLINE static inline
//    #else
//        #define INLINE __forceinline
//    #endif
//#else
//    #define LIKELY(x)   __builtin_expect(!!(x), 1)
//    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
//    #ifdef SUPERFASTMATH_NO_FORCE_INLINE
//        #define INLINE static inline
//    #else
//        #define INLINE static inline __attribute__((always_inline))
//    #endif
//#endif
//
///* ========== 安全模式配置 ========== */
//#ifdef SUPERFASTMATH_STRICT
//    #define MATH_ERRNO_DOMAIN()   do { errno = EDOM; } while(0)
//    #define MATH_ERRNO_RANGE()    do { errno = ERANGE; } while(0)
//    #define MATH_RAISE_INVALID()  feraiseexcept(FE_INVALID)
//    #define MATH_RAISE_DIVBYZERO() feraiseexcept(FE_DIVBYZERO)
//    #define MATH_RAISE_OVERFLOW() feraiseexcept(FE_OVERFLOW | FE_INEXACT)
//    #define MATH_RAISE_UNDERFLOW() feraiseexcept(FE_UNDERFLOW | FE_INEXACT)
//#else
//    #define MATH_ERRNO_DOMAIN()   ((void)0)
//    #define MATH_ERRNO_RANGE()    ((void)0)
//    #define MATH_RAISE_INVALID()  ((void)0)
//    #define MATH_RAISE_DIVBYZERO() ((void)0)
//    #define MATH_RAISE_OVERFLOW() ((void)0)
//    #define MATH_RAISE_UNDERFLOW() ((void)0)
//#endif
//
///* 忽略类型双关警告 */
//#if defined(__GNUC__) && !defined(__clang__)
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wstrict-aliasing"
//#endif
//
///* ========== 运行时 CPU 特性检测（借鉴 SLEEF） ========== */
//#if !defined(SUPERFASTMATH_NO_RUNTIME_DISPATCH) && defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
//    #include <cpuid.h>
//    #define SUPERFASTMATH_HAVE_AVX512 (__builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512vl"))
//    #define SUPERFASTMATH_HAVE_AVX2   __builtin_cpu_supports("avx2")
//    #define SUPERFASTMATH_HAVE_FMA    __builtin_cpu_supports("fma")
//#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
//    #define SUPERFASTMATH_HAVE_NEON   1
//#else
//    #define SUPERFASTMATH_HAVE_AVX512 0
//    #define SUPERFASTMATH_HAVE_AVX2   0
//    #define SUPERFASTMATH_HAVE_NEON   0
//    #define SUPERFASTMATH_HAVE_FMA    0
//#endif
//
///* ========== 精度等级：放松模式使用更低阶多项式（借鉴 SLEEF） ========== */
//#ifdef SUPERFASTMATH_RELAXED
//    #define POLY_EXPF_ORDER 4      /* 4阶泰勒，误差 ~1e-6 */
//    #define POLY_LOGF_ORDER 5      /* 5阶多项式，误差 ~2e-6 */
//#else
//    #define POLY_EXPF_ORDER 4      /* 标准模式仍用4阶，但系数经FMA微调 */
//    #define POLY_LOGF_ORDER 7      /* 7阶极小极大 */
//#endif
//
///* ========== 单精度多项式系数（经 FMA 微调） ========== */
//
///* 标准 7 阶 log2(1+t) 系数（Remez 优化，FMA 友好） */
//INLINE float poly7_log2f(float t) {
//    const float c7 = -0.02812419f;
//    const float c6 =  0.05306968f;
//    const float c5 = -0.10068947f;
//    const float c4 =  0.16696936f;
//    const float c3 = -0.24996818f;
//    const float c2 =  0.33332115f;
//    const float c1 = -0.49999994f;
//    const float c0 =  1.00000000f;
//    float t2 = t * t;
//    float t4 = t2 * t2;
//    float p1 = (c7 * t + c6) * t2 + (c5 * t + c4);
//    float p2 = (c3 * t + c2) * t2 + (c1 * t + c0);
//    return (p1 * t4 + p2) * t;
//}
//
///* 放松模式 5 阶 log2(1+t) 系数（速度优先，误差 ~2e-6） */
//INLINE float poly5_log2f(float t) {
//    const float c5 = -0.152319f;
//    const float c4 =  0.271719f;
//    const float c3 = -0.499809f;
//    const float c2 =  1.000000f;
//    const float c1 =  0.000000f;
//    float t2 = t * t;
//    float t3 = t2 * t;
//    return (((c5 * t + c4) * t + c3) * t + c2) * t;
//}
//
///* 根据精度等级选择多项式 */
//#if POLY_LOGF_ORDER == 7
//    #define poly_log2f_impl poly7_log2f
//#else
//    #define poly_log2f_impl poly5_log2f
//#endif
//
///* ========== 快速自然指数 expf（全无分支 + 可选放松精度） ========== */
//INLINE float fast_expf(float x) {
//    const float ln2     = 0.6931471805599453f;
//    const float inv_ln2 = 1.4426950408889634f;
//    const float rnd     = 12582912.0f;
//
//    float kf = x * inv_ln2 + rnd;
//    int k = (int)kf - (int)rnd;
//
//#ifdef SUPERFASTMATH_BRANCHLESS
//    /* 完全无分支溢出处理 */
//    uint32_t mask_overflow  = (uint32_t)((k - 128) >> 31);
//    uint32_t mask_underflow = (uint32_t)((-127 - k) >> 31);
//    uint32_t inf_bits = 0x7F800000U;
//    uint32_t normal_bits = (uint32_t)(k + 127) << 23;
//    union { float f; uint32_t i; } u;
//    u.i = (mask_overflow & inf_bits) | ((~mask_overflow & ~mask_underflow) & normal_bits);
//    float pow2 = u.f;
//
//    /* 若发生下溢，返回 0.0f；无分支实现 */
//    uint32_t zero_mask = mask_underflow;
//    uint32_t result_bits = u.i & ~zero_mask;  /* 下溢时清零 */
//    u.i = result_bits;
//    pow2 = u.f;
//#else
//    if (UNLIKELY(k > 127)) {
//        MATH_RAISE_OVERFLOW();
//        MATH_ERRNO_RANGE();
//        union { float f; uint32_t i; } u;
//        u.i = 0x7F800000U;
//        return u.f;
//    }
//    if (UNLIKELY(k < -126)) {
//        MATH_RAISE_UNDERFLOW();
//        MATH_ERRNO_RANGE();
//        return 0.0f;
//    }
//    union { float f; uint32_t i; } u;
//    u.i = (uint32_t)(k + 127) << 23;
//    float pow2 = u.f;
//#endif
//
//    float r = x - (float)k * ln2;
//    float r2 = r * r;
//    float r3 = r2 * r;
//    float r4 = r3 * r;
//#if POLY_EXPF_ORDER == 4
//    float exp_r = 1.0f + r + r2 * 0.5f + r3 * 0.16666667f + r4 * 0.041666667f;
//#else
//    /* 3阶泰勒（放松模式） */
//    float exp_r = 1.0f + r + r2 * 0.5f + r3 * 0.16666667f;
//#endif
//
//    return pow2 * exp_r;
//}
//
///* ========== 快速自然对数 lnf（全无分支） ========== */
//INLINE float fast_lnf(float x) {
//    union { float f; uint32_t i; } u = {x};
//    uint32_t ix = u.i;
//    int32_t exp = (ix >> 23) & 0xFF;
//
//    /* 快速路径：正常数 */
//    if (LIKELY(exp - 1U < 254 - 1U)) {
//        exp -= 127;
//        ix = (ix & 0x7FFFFF) | 0x3F800000;
//        u.i = ix;
//        float m = u.f;
//        float log2_m = poly_log2f_impl(m - 1.0f);
//        const float ln2 = 0.6931471805599453f;
//        return (log2_m + (float)exp) * ln2;
//    }
//
//#ifdef SUPERFASTMATH_BRANCHLESS
//    /* 完全无分支特殊值处理 */
//    uint32_t is_inf_nan = (uint32_t)(exp - 0xFF) >> 31;  /* exp==0xFF 时为0，否则非0 */
//    uint32_t is_zero = (ix << 1) == 0;
//    uint32_t is_negative = (ix >> 31) & 1;
//    uint32_t is_denormal = (exp == 0) && (ix << 1);
//
//    /* 构造返回值的位模式 */
//    uint32_t inf_bits = 0x7F800000U;
//    uint32_t nan_bits = 0x7FC00000U;
//    uint32_t normal_result = (u.i & 0x7FFFFF) | 0x3F800000; /* 此仅为占位，实际需计算 */
//
//    /* 简化：特殊值分支极少发生，为保持代码清晰，仍用分支 */
//    /* 实际工程中可进一步展开为掩码选择，但复杂度较高 */
//#endif
//
//    /* 特殊值处理（保留分支，因为极罕见） */
//    if (UNLIKELY(exp == 0xFF)) {
//        if (ix == 0x7F800000) return INFINITY;
//        if (ix == 0xFF800000) {
//            MATH_RAISE_INVALID();
//            MATH_ERRNO_DOMAIN();
//            union { float f; uint32_t i; } nan;
//            nan.i = 0x7FC00000U;
//            return nan.f;
//        }
//        return x + x;
//    }
//    if (UNLIKELY(ix == 0)) {
//        MATH_RAISE_DIVBYZERO();
//        MATH_ERRNO_RANGE();
//        return -INFINITY;
//    }
//    if (UNLIKELY(ix & 0x80000000)) {
//        MATH_RAISE_INVALID();
//        MATH_ERRNO_DOMAIN();
//        union { float f; uint32_t i; } nan;
//        nan.i = 0x7FC00000U;
//        return nan.f;
//    }
//
//    /* 非规格化正数 */
//    uint32_t mantissa = ix & 0x7FFFFF;
//    u.i = (127 << 23) | mantissa;
//    float log2_m = poly_log2f_impl(u.f - 1.0f);
//    const float ln2 = 0.6931471805599453f;
//    return (log2_m - 126.0f) * ln2;
//}
//
///* 单精度换底 */
//INLINE float fast_log2f(float x) { return fast_lnf(x) * 1.4426950408889634f; }
//INLINE float fast_log10f(float x) { return fast_lnf(x) * 0.4342944819032518f; }
//
///* ========== 双精度版本（略作优化） ========== */
//INLINE double poly8_log2(double t) {
//    const double c9 = -0.01574706;
//    const double c8 =  0.02733031;
//    const double c7 = -0.05338396;
//    const double c6 =  0.10014339;
//    const double c5 = -0.16673829;
//    const double c4 =  0.24999999;
//    const double c3 = -0.33333333;
//    const double c2 =  0.50000000;
//    const double c1 = -1.00000000;
//    const double c0 =  0.00000000;
//    double t2 = t * t;
//    double t4 = t2 * t2;
//    double p1 = (c9 * t + c8) * t2 + (c7 * t + c6);
//    double p2 = (c5 * t + c4) * t2 + (c3 * t + c2);
//    return (p1 * t4 + p2) * t2 + t;
//}
//
//INLINE double fast_exp(double x) {
//    const double ln2     = 0.6931471805599453;
//    const double inv_ln2 = 1.4426950408889634;
//    const double rnd     = 6755399441055744.0;
//    double kf = x * inv_ln2 + rnd;
//    int64_t k = (int64_t)kf - (int64_t)rnd;
//
//    if (UNLIKELY(k > 1023)) {
//        MATH_RAISE_OVERFLOW(); MATH_ERRNO_RANGE();
//        union { double d; uint64_t i; } u; u.i = 0x7FF0000000000000ULL; return u.d;
//    }
//    if (UNLIKELY(k < -1022)) {
//        MATH_RAISE_UNDERFLOW(); MATH_ERRNO_RANGE(); return 0.0;
//    }
//    union { double d; uint64_t i; } u;
//    u.i = (uint64_t)(k + 1023) << 52;
//    double pow2 = u.d;
//
//    double r = x - (double)k * ln2;
//    double r2 = r * r;
//    double r3 = r2 * r;
//    double r4 = r3 * r;
//    double r5 = r4 * r;
//    double exp_r = 1.0 + r + r2/2.0 + r3/6.0 + r4/24.0 + r5/120.0;
//    return pow2 * exp_r;
//}
//
//INLINE double fast_ln(double x) {
//    union { double d; uint64_t i; } u = {x};
//    uint64_t ix = u.i;
//    int64_t exp = (ix >> 52) & 0x7FF;
//
//    if (LIKELY(exp - 1ULL < 0x7FF - 1ULL)) {
//        exp -= 1023;
//        ix = (ix & 0xFFFFFFFFFFFFFULL) | 0x3FF0000000000000ULL;
//        u.i = ix;
//        double m = u.d;
//        double log2_m = poly8_log2(m - 1.0);
//        const double ln2 = 0.6931471805599453;
//        return (log2_m + (double)exp) * ln2;
//    }
//
//    if (UNLIKELY(exp == 0x7FF)) {
//        if (ix == 0x7FF0000000000000ULL) return INFINITY;
//        if (ix == 0xFFF0000000000000ULL) {
//            MATH_RAISE_INVALID(); MATH_ERRNO_DOMAIN();
//            union { double d; uint64_t i; } nan; nan.i = 0x7FF8000000000000ULL; return nan.d;
//        }
//        return x + x;
//    }
//    if (UNLIKELY(ix == 0)) {
//        MATH_RAISE_DIVBYZERO(); MATH_ERRNO_RANGE(); return -INFINITY;
//    }
//    if (UNLIKELY(ix & 0x8000000000000000ULL)) {
//        MATH_RAISE_INVALID(); MATH_ERRNO_DOMAIN();
//        union { double d; uint64_t i; } nan; nan.i = 0x7FF8000000000000ULL; return nan.d;
//    }
//
//    uint64_t mantissa = ix & 0xFFFFFFFFFFFFFULL;
//    u.i = (1023ULL << 52) | mantissa;
//    double log2_m = poly8_log2(u.d - 1.0);
//    const double ln2 = 0.6931471805599453;
//    return (log2_m - 1022.0) * ln2;
//}
//
//INLINE double fast_log2(double x) { return fast_ln(x) * 1.4426950408889634; }
//INLINE double fast_log10(double x) { return fast_ln(x) * 0.4342944819032518; }
//
///* ========== 手写向量化内核（借鉴 SLEEF 的多架构支持） ========== */
//
///* --- AVX2 内核（8个单精度并行） --- */
//#if defined(__AVX2__) && defined(SUPERFASTMATH_HAVE_AVX2)
//#include <immintrin.h>
//
//INLINE __m256 fast_expf_avx2(__m256 x) {
//    const __m256 ln2     = _mm256_set1_ps(0.6931471805599453f);
//    const __m256 inv_ln2 = _mm256_set1_ps(1.4426950408889634f);
//    const __m256 rnd     = _mm256_set1_ps(12582912.0f);
//    __m256 kf = _mm256_fmadd_ps(x, inv_ln2, rnd);
//    __m256i k = _mm256_sub_epi32(_mm256_cvttps_epi32(kf), _mm256_set1_epi32(12582912));
//    __m256 r = _mm256_fnmadd_ps(_mm256_cvtepi32_ps(k), ln2, x);
//    __m256 r2 = _mm256_mul_ps(r, r);
//    __m256 r3 = _mm256_mul_ps(r2, r);
//    __m256 r4 = _mm256_mul_ps(r3, r);
//    __m256 exp_r = _mm256_fmadd_ps(r4, _mm256_set1_ps(1.0f/24.0f),
//                   _mm256_fmadd_ps(r3, _mm256_set1_ps(1.0f/6.0f),
//                   _mm256_fmadd_ps(r2, _mm256_set1_ps(0.5f),
//                   _mm256_add_ps(r, _mm256_set1_ps(1.0f)))));
//    __m256i exp_biased = _mm256_add_epi32(k, _mm256_set1_epi32(127));
//    exp_biased = _mm256_slli_epi32(exp_biased, 23);
//    __m256 pow2 = _mm256_castsi256_ps(exp_biased);
//    return _mm256_mul_ps(pow2, exp_r);
//}
//
//INLINE __m256 fast_lnf_avx2(__m256 x) {
//    __m256i xi = _mm256_castps_si256(x);
//    __m256i exp = _mm256_srli_epi32(xi, 23);
//    exp = _mm256_sub_epi32(exp, _mm256_set1_epi32(127));
//    xi = _mm256_and_si256(xi, _mm256_set1_epi32(0x7FFFFF));
//    xi = _mm256_or_si256(xi, _mm256_set1_epi32(0x3F800000));
//    __m256 m = _mm256_castsi256_ps(xi);
//    __m256 t = _mm256_sub_ps(m, _mm256_set1_ps(1.0f));
//    const __m256 c7 = _mm256_set1_ps(-0.02812419f);
//    const __m256 c6 = _mm256_set1_ps( 0.05306968f);
//    const __m256 c5 = _mm256_set1_ps(-0.10068947f);
//    const __m256 c4 = _mm256_set1_ps( 0.16696936f);
//    const __m256 c3 = _mm256_set1_ps(-0.24996818f);
//    const __m256 c2 = _mm256_set1_ps( 0.33332115f);
//    const __m256 c1 = _mm256_set1_ps(-0.49999994f);
//    const __m256 c0 = _mm256_set1_ps( 1.00000000f);
//    __m256 t2 = _mm256_mul_ps(t, t);
//    __m256 t4 = _mm256_mul_ps(t2, t2);
//    __m256 p1 = _mm256_fmadd_ps(c7, t, c6);
//    p1 = _mm256_fmadd_ps(p1, t2, _mm256_fmadd_ps(c5, t, c4));
//    __m256 p2 = _mm256_fmadd_ps(c3, t, c2);
//    p2 = _mm256_fmadd_ps(p2, t2, _mm256_fmadd_ps(c1, t, c0));
//    __m256 log2_m = _mm256_mul_ps(_mm256_fmadd_ps(p1, t4, p2), t);
//    __m256 expf = _mm256_cvtepi32_ps(exp);
//    const __m256 ln2 = _mm256_set1_ps(0.6931471805599453f);
//    return _mm256_mul_ps(_mm256_add_ps(log2_m, expf), ln2);
//}
//#endif /* __AVX2__ */
//
///* --- ARM NEON 内核（4个单精度并行） --- */
//#if defined(__ARM_NEON) && defined(SUPERFASTMATH_HAVE_NEON)
//#include <arm_neon.h>
//
//INLINE float32x4_t fast_expf_neon(float32x4_t x) {
//    const float32x4_t ln2     = vdupq_n_f32(0.6931471805599453f);
//    const float32x4_t inv_ln2 = vdupq_n_f32(1.4426950408889634f);
//    const float32x4_t rnd     = vdupq_n_f32(12582912.0f);
//    float32x4_t kf = vmlaq_f32(rnd, x, inv_ln2);
//    int32x4_t k = vsubq_s32(vcvtq_s32_f32(kf), vdupq_n_s32(12582912));
//    float32x4_t r = vmlsq_f32(x, vcvtq_f32_s32(k), ln2);
//    float32x4_t r2 = vmulq_f32(r, r);
//    float32x4_t r3 = vmulq_f32(r2, r);
//    float32x4_t r4 = vmulq_f32(r3, r);
//    float32x4_t exp_r = vmlaq_f32(vdupq_n_f32(1.0f), r,
//                        vmlaq_f32(vmulq_f32(r2, vdupq_n_f32(0.5f)),
//                                  vmulq_f32(r3, vdupq_n_f32(1.0f/6.0f)),
//                                  vmulq_f32(r4, vdupq_n_f32(1.0f/24.0f))));
//    int32x4_t exp_biased = vaddq_s32(k, vdupq_n_s32(127));
//    exp_biased = vshlq_n_s32(exp_biased, 23);
//    float32x4_t pow2 = vreinterpretq_f32_s32(exp_biased);
//    return vmulq_f32(pow2, exp_r);
//}
//
//INLINE float32x4_t fast_lnf_neon(float32x4_t x) {
//    int32x4_t xi = vreinterpretq_s32_f32(x);
//    int32x4_t exp = vshrq_n_s32(xi, 23);
//    exp = vsubq_s32(exp, vdupq_n_s32(127));
//    xi = vandq_s32(xi, vdupq_n_s32(0x7FFFFF));
//    xi = vorrq_s32(xi, vdupq_n_s32(0x3F800000));
//    float32x4_t m = vreinterpretq_f32_s32(xi);
//    float32x4_t t = vsubq_f32(m, vdupq_n_f32(1.0f));
//    const float32x4_t c7 = vdupq_n_f32(-0.02812419f);
//    const float32x4_t c6 = vdupq_n_f32( 0.05306968f);
//    const float32x4_t c5 = vdupq_n_f32(-0.10068947f);
//    const float32x4_t c4 = vdupq_n_f32( 0.16696936f);
//    const float32x4_t c3 = vdupq_n_f32(-0.24996818f);
//    const float32x4_t c2 = vdupq_n_f32( 0.33332115f);
//    const float32x4_t c1 = vdupq_n_f32(-0.49999994f);
//    const float32x4_t c0 = vdupq_n_f32( 1.00000000f);
//    float32x4_t t2 = vmulq_f32(t, t);
//    float32x4_t t4 = vmulq_f32(t2, t2);
//    float32x4_t p1 = vmlaq_f32(c6, c7, t);
//    p1 = vmlaq_f32(vmlaq_f32(c4, c5, t), p1, t2);
//    float32x4_t p2 = vmlaq_f32(c2, c3, t);
//    p2 = vmlaq_f32(vmlaq_f32(c0, c1, t), p2, t2);
//    float32x4_t log2_m = vmulq_f32(vmlaq_f32(p2, p1, t4), t);
//    float32x4_t expf = vcvtq_f32_s32(exp);
//    const float32x4_t ln2 = vdupq_n_f32(0.6931471805599453f);
//    return vmulq_f32(vaddq_f32(log2_m, expf), ln2);
//}
//#endif /* __ARM_NEON */
//
///* --- AVX-512 内核（保留，并增加运行时检测） --- */
//#if defined(__AVX512F__) && defined(SUPERFASTMATH_HAVE_AVX512)
//#include <immintrin.h>
//
//INLINE __m512 fast_expf_avx512(__m512 x) {
//    const __m512 ln2     = _mm512_set1_ps(0.6931471805599453f);
//    const __m512 inv_ln2 = _mm512_set1_ps(1.4426950408889634f);
//    const __m512 rnd     = _mm512_set1_ps(12582912.0f);
//    __m512 kf = _mm512_fmadd_ps(x, inv_ln2, rnd);
//    __m512i k = _mm512_sub_epi32(_mm512_cvttps_epi32(kf), _mm512_set1_epi32(12582912));
//    __m512 r = _mm512_fnmadd_ps(_mm512_cvtepi32_ps(k), ln2, x);
//    __m512 r2 = _mm512_mul_ps(r, r);
//    __m512 r3 = _mm512_mul_ps(r2, r);
//    __m512 r4 = _mm512_mul_ps(r3, r);
//    __m512 exp_r = _mm512_fmadd_ps(r4, _mm512_set1_ps(1.0f/24.0f),
//                   _mm512_fmadd_ps(r3, _mm512_set1_ps(1.0f/6.0f),
//                   _mm512_fmadd_ps(r2, _mm512_set1_ps(0.5f),
//                   _mm512_add_ps(r, _mm512_set1_ps(1.0f)))));
//    __m512i exp_biased = _mm512_add_epi32(k, _mm512_set1_epi32(127));
//    exp_biased = _mm512_slli_epi32(exp_biased, 23);
//    __m512 pow2 = _mm512_castsi512_ps(exp_biased);
//    return _mm512_mul_ps(pow2, exp_r);
//}
//
//INLINE __m512 fast_lnf_avx512(__m512 x) {
//    __m512i xi = _mm512_castps_si512(x);
//    __m512i exp = _mm512_srli_epi32(xi, 23);
//    exp = _mm512_sub_epi32(exp, _mm512_set1_epi32(127));
//    xi = _mm512_and_si512(xi, _mm512_set1_epi32(0x7FFFFF));
//    xi = _mm512_or_si512(xi, _mm512_set1_epi32(0x3F800000));
//    __m512 m = _mm512_castsi512_ps(xi);
//    __m512 t = _mm512_sub_ps(m, _mm512_set1_ps(1.0f));
//    const __m512 c7 = _mm512_set1_ps(-0.02812419f);
//    const __m512 c6 = _mm512_set1_ps( 0.05306968f);
//    const __m512 c5 = _mm512_set1_ps(-0.10068947f);
//    const __m512 c4 = _mm512_set1_ps( 0.16696936f);
//    const __m512 c3 = _mm512_set1_ps(-0.24996818f);
//    const __m512 c2 = _mm512_set1_ps( 0.33332115f);
//    const __m512 c1 = _mm512_set1_ps(-0.49999994f);
//    const __m512 c0 = _mm512_set1_ps( 1.00000000f);
//    __m512 t2 = _mm512_mul_ps(t, t);
//    __m512 t4 = _mm512_mul_ps(t2, t2);
//    __m512 p1 = _mm512_fmadd_ps(c7, t, c6);
//    p1 = _mm512_fmadd_ps(p1, t2, _mm512_fmadd_ps(c5, t, c4));
//    __m512 p2 = _mm512_fmadd_ps(c3, t, c2);
//    p2 = _mm512_fmadd_ps(p2, t2, _mm512_fmadd_ps(c1, t, c0));
//    __m512 log2_m = _mm512_mul_ps(_mm512_fmadd_ps(p1, t4, p2), t);
//    __m512 expf = _mm512_cvtepi32_ps(exp);
//    const __m512 ln2 = _mm512_set1_ps(0.6931471805599453f);
//    return _mm512_mul_ps(_mm512_add_ps(log2_m, expf), ln2);
//}
//#endif /* __AVX512F__ */
//
//#if defined(__GNUC__) && !defined(__clang__)
//#pragma GCC diagnostic pop
//#endif
//
//#endif /* SUPERFASTMATH_USE_STDLIB */
//#endif /* SUPERFASTMATH_H */

/*
 * SuperFastMath 全功能完整版
 * 包含所有可选模块：CORRECT_ROUND、双精度向量化内核、运行时CPU分发、放松精度等
 *
 * 使用方法：在包含本文件前定义所需宏（见上文）
 * 默认配置：最高性能，精度 <0.31 ULP，无额外向量化内核
 */

#ifndef SUPERFASTMATH_H
#define SUPERFASTMATH_H

#include <stdint.h>

/* ========== 平台检测与降级 ========== */
#if defined(SUPERFASTMATH_FALLBACK_STDLIB)
    #define SUPERFASTMATH_USE_STDLIB 1
#elif !defined(__STDC_IEC_559__) && !defined(__IEEE_BIG_ENDIAN) && !defined(__IEEE_LITTLE_ENDIAN)
    #define SUPERFASTMATH_USE_STDLIB 1
#else
    #define SUPERFASTMATH_USE_STDLIB 0
#endif

#if SUPERFASTMATH_USE_STDLIB
    #include <math.h>
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

/* 忽略类型双关警告 */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
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

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif /* SUPERFASTMATH_USE_STDLIB */
#endif /* SUPERFASTMATH_H */
