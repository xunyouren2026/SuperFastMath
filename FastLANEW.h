
#ifndef FAST_GEMM_H
#define FAST_GEMM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* --------------------------------------------------------------------------
 * 编译器兼容性与内联宏
 * ------------------------------------------------------------------------ */
#ifdef _MSC_VER
#include <intrin.h>
#define FG_INLINE static __forceinline
#define FG_ALIGNED(x) __declspec(align(x))
#define FG_RESTRICT __restrict
#define FG_PREFETCH(addr) _mm_prefetch((const char *)(addr), _MM_HINT_T0)
#define FG_ALLOCA(s) _alloca(s)
#else
#include <x86intrin.h>
#include <alloca.h>
#define FG_INLINE static inline __attribute__((always_inline))
#define FG_ALIGNED(x) __attribute__((aligned(x)))
#define FG_RESTRICT __restrict__
#define FG_PREFETCH(addr) __builtin_prefetch((addr), 0, 3)
#define FG_ALLOCA(s) alloca(s)
#endif

    /* --------------------------------------------------------------------------
 * CPU 特性检测 + 缓存大小探测
 * ------------------------------------------------------------------------ */
    typedef struct
    {
        int avx512f, avx2, fma, neon;
        int avx512fp16, avx512bf16;
        int amx_bf16, amx_int8;
        int l1d_cache_size, l2_cache_size, l3_cache_size;
    } fg_cpu_t;

    static fg_cpu_t g_fg_cpu = {0};
    static int g_fg_cpu_init = 0;

#if defined(__x86_64__) || defined(__i386__) || defined(_M_AMD64) || defined(_M_IX86)
    static void fg_detect_cache_size(void)
    {
        uint32_t eax, ebx, ecx, edx;
        __cpuid(0x80000005, (int *)&eax, (int *)&ebx, (int *)&ecx, (int *)&edx);
        g_fg_cpu.l1d_cache_size = (ecx >> 24) * 1024;
        int l1_found = 0, l2_found = 0, l3_found = 0;
        for (int i = 0; i < 4; ++i)
        {
            __cpuidex((int *)&eax, (int *)&ebx, (int *)&ecx, (int *)&edx, 0x04, i);
            int cache_type = eax & 0x1F;
            if (cache_type == 0)
                break;
            int level = (eax >> 5) & 0x7;
            int ways = ((ebx >> 22) & 0x3FF) + 1;
            int partitions = ((ebx >> 12) & 0x3FF) + 1;
            int line_size = (ebx & 0xFFF) + 1;
            int sets = ecx + 1;
            int size = ways * partitions * line_size * sets;
            if (level == 1 && cache_type == 1 && !l1_found)
            {
                g_fg_cpu.l1d_cache_size = size;
                l1_found = 1;
            }
            else if (level == 2 && !l2_found)
            {
                g_fg_cpu.l2_cache_size = size;
                l2_found = 1;
            }
            else if (level == 3 && !l3_found)
            {
                g_fg_cpu.l3_cache_size = size;
                l3_found = 1;
            }
        }
        if (!l1_found)
            g_fg_cpu.l1d_cache_size = 32 * 1024;
        if (!l2_found)
            g_fg_cpu.l2_cache_size = 256 * 1024;
        if (!l3_found)
            g_fg_cpu.l3_cache_size = 2048 * 1024;
    }
#else
static void fg_detect_cache_size(void)
{
    g_fg_cpu.l1d_cache_size = 64 * 1024;
    g_fg_cpu.l2_cache_size = 512 * 1024;
    g_fg_cpu.l3_cache_size = 4096 * 1024;
}
#endif

    FG_INLINE void fg_detect_cpu(void)
    {
        if (g_fg_cpu_init)
            return;
        g_fg_cpu_init = 1;
#if defined(__x86_64__) || defined(__i386__) || defined(_M_AMD64) || defined(_M_IX86)
#ifdef __GNUC__
        g_fg_cpu.avx2 = __builtin_cpu_supports("avx2");
        g_fg_cpu.fma = __builtin_cpu_supports("fma");
        g_fg_cpu.avx512f = __builtin_cpu_supports("avx512f");
        g_fg_cpu.avx512fp16 = __builtin_cpu_supports("avx512fp16");
        g_fg_cpu.avx512bf16 = __builtin_cpu_supports("avx512bf16");
        g_fg_cpu.amx_bf16 = __builtin_cpu_supports("amx-bf16");
        g_fg_cpu.amx_int8 = __builtin_cpu_supports("amx-int8");
#elif defined(_MSC_VER)
        int info[4];
        __cpuid(info, 0);
        int nIds = info[0];
        if (nIds >= 7)
        {
            __cpuidex(info, 7, 0);
            g_fg_cpu.avx2 = (info[1] & (1 << 5)) != 0;
            g_fg_cpu.avx512f = (info[1] & (1 << 16)) != 0;
            g_fg_cpu.avx512fp16 = (info[3] & (1 << 23)) != 0;
            g_fg_cpu.avx512bf16 = (info[3] & (1 << 5)) != 0;
            g_fg_cpu.amx_bf16 = (info[3] & (1 << 22)) != 0;
            g_fg_cpu.amx_int8 = (info[3] & (1 << 24)) != 0;
        }
        __cpuid(info, 1);
        g_fg_cpu.fma = (info[2] & (1 << 12)) != 0;
#endif
        fg_detect_cache_size();
#elif defined(__ARM_NEON)
    g_fg_cpu.neon = 1;
    g_fg_cpu.l1d_cache_size = 64 * 1024;
    g_fg_cpu.l2_cache_size = 512 * 1024;
    g_fg_cpu.l3_cache_size = 4096 * 1024;
#endif
    }

#define FG_CPU_AVX512() (fg_detect_cpu(), g_fg_cpu.avx512f)
#define FG_CPU_AVX2() (fg_detect_cpu(), g_fg_cpu.avx2)
#define FG_CPU_FMA() (fg_detect_cpu(), g_fg_cpu.fma)
#define FG_CPU_NEON() (fg_detect_cpu(), g_fg_cpu.neon)
#define FG_CPU_AVX512FP16() (fg_detect_cpu(), g_fg_cpu.avx512fp16)
#define FG_CPU_AVX512BF16() (fg_detect_cpu(), g_fg_cpu.avx512bf16)
#define FG_CPU_AMX_BF16() (fg_detect_cpu(), g_fg_cpu.amx_bf16)
#define FG_CPU_AMX_INT8() (fg_detect_cpu(), g_fg_cpu.amx_int8)

    /* --------------------------------------------------------------------------
 * 自动调优参数表
 * ------------------------------------------------------------------------ */
    typedef struct
    {
        int mr, nr, kr;
        int mc, nc, kc;
    } fg_gemm_params_t;
    static fg_gemm_params_t g_fg_params_sp, g_fg_params_dp;
    static int g_fg_params_inited = 0;

    static void fg_auto_tune_params(void)
    {
        if (g_fg_params_inited)
            return;
        fg_detect_cpu();
        int l1 = g_fg_cpu.l1d_cache_size / sizeof(float);
        int l2 = g_fg_cpu.l2_cache_size / sizeof(float);
        int l3 = g_fg_cpu.l3_cache_size / sizeof(float);

        g_fg_params_sp.mr = 6;
        g_fg_params_sp.nr = (FG_CPU_AVX512() || FG_CPU_AVX2()) ? 16 : 8;
        g_fg_params_sp.kr = 256;
        g_fg_params_sp.mc = (l2 / g_fg_params_sp.kr) & ~(g_fg_params_sp.mr - 1);
        if (g_fg_params_sp.mc < g_fg_params_sp.mr)
            g_fg_params_sp.mc = g_fg_params_sp.mr;
        g_fg_params_sp.nc = (l3 / g_fg_params_sp.kr) & ~(g_fg_params_sp.nr - 1);
        g_fg_params_sp.kc = (l1 / (g_fg_params_sp.mr * g_fg_params_sp.nr)) * g_fg_params_sp.nr;
        if (g_fg_params_sp.kc < g_fg_params_sp.kr)
            g_fg_params_sp.kc = g_fg_params_sp.kr;

        l1 = g_fg_cpu.l1d_cache_size / sizeof(double);
        l2 = g_fg_cpu.l2_cache_size / sizeof(double);
        l3 = g_fg_cpu.l3_cache_size / sizeof(double);
        g_fg_params_dp.mr = 4;
        g_fg_params_dp.nr = (FG_CPU_AVX512() || FG_CPU_AVX2()) ? 8 : 4;
        g_fg_params_dp.kr = 256;
        g_fg_params_dp.mc = (l2 / g_fg_params_dp.kr) & ~(g_fg_params_dp.mr - 1);
        if (g_fg_params_dp.mc < g_fg_params_dp.mr)
            g_fg_params_dp.mc = g_fg_params_dp.mr;
        g_fg_params_dp.nc = (l3 / g_fg_params_dp.kr) & ~(g_fg_params_dp.nr - 1);
        g_fg_params_dp.kc = (l1 / (g_fg_params_dp.mr * g_fg_params_dp.nr)) * g_fg_params_dp.nr;
        if (g_fg_params_dp.kc < g_fg_params_dp.kr)
            g_fg_params_dp.kc = g_fg_params_dp.kr;

        g_fg_params_inited = 1;
    }

#define FG_MR_SP (fg_auto_tune_params(), g_fg_params_sp.mr)
#define FG_NR_SP (fg_auto_tune_params(), g_fg_params_sp.nr)
#define FG_KR_SP (fg_auto_tune_params(), g_fg_params_sp.kr)
#define FG_MC_SP (fg_auto_tune_params(), g_fg_params_sp.mc)
#define FG_NC_SP (fg_auto_tune_params(), g_fg_params_sp.nc)
#define FG_KC_SP (fg_auto_tune_params(), g_fg_params_sp.kc)

#define FG_MR_DP (fg_auto_tune_params(), g_fg_params_dp.mr)
#define FG_NR_DP (fg_auto_tune_params(), g_fg_params_dp.nr)
#define FG_KR_DP (fg_auto_tune_params(), g_fg_params_dp.kr)
#define FG_MC_DP (fg_auto_tune_params(), g_fg_params_dp.mc)
#define FG_NC_DP (fg_auto_tune_params(), g_fg_params_dp.nc)
#define FG_KC_DP (fg_auto_tune_params(), g_fg_params_dp.kc)

#define FG_STACK_LIMIT (1024 * 1024)

    /* --------------------------------------------------------------------------
 * 类型定义 (FP16/BF16)
 * ------------------------------------------------------------------------ */
    typedef uint16_t fg_float16_t;
    typedef uint16_t fg_bfloat16_t;

    /* --------------------------------------------------------------------------
 * 公共接口声明
 * ------------------------------------------------------------------------ */
    void fast_sgemm(bool transA, bool transB, int M, int N, int K, float alpha,
                    const float *A, int lda, const float *B, int ldb, float beta, float *C, int ldc);
    void fast_dgemm(bool transA, bool transB, int M, int N, int K, double alpha,
                    const double *A, int lda, const double *B, int ldb, double beta, double *C, int ldc);
    void fast_hgemm(bool transA, bool transB, int M, int N, int K, float alpha,
                    const fg_float16_t *A, int lda, const fg_float16_t *B, int ldb, float beta, float *C, int ldc);
    void fast_bgemm(bool transA, bool transB, int M, int N, int K, float alpha,
                    const fg_bfloat16_t *A, int lda, const fg_bfloat16_t *B, int ldb, float beta, float *C, int ldc);
    void fast_lowrank_gemm(bool transA, bool transB, int M, int N, int K, float alpha,
                           const float *A, int lda, const float *B, int ldb, float beta, float *C, int ldc, int rank);
    void fast_strassen_gemm(int n, float alpha, const float *A, int lda,
                            const float *B, int ldb, float beta, float *C, int ldc);
    void fast_amx_bf16_gemm(bool transA, bool transB, int M, int N, int K, float alpha,
                            const fg_bfloat16_t *A, int lda, const fg_bfloat16_t *B, int ldb, float beta, float *C, int ldc);
    void fast_sgemm_batched(int batch_count, bool transA, bool transB, int M, int N, int K, float alpha,
                            const float *const *A, int lda, const float *const *B, int ldb, float beta, float *const *C, int ldc);
    void fast_dgemm_batched(int batch_count, bool transA, bool transB, int M, int N, int K, double alpha,
                            const double *const *A, int lda, const double *const *B, int ldb, double beta, double *const *C, int ldc);

    /* 稀疏矩阵 CSR */
    typedef struct
    {
        int n_rows;
        int n_cols;
        int nnz;
        int *row_ptr;
        int *col_idx;
        float *values;
    } fg_csr_matrix_t;

    void fg_spmv_csr(const fg_csr_matrix_t *A, const float *x, float *y, float alpha, float beta);
    void fg_spmm_csr(const fg_csr_matrix_t *A, const float *B, int ldb, float *Y, int ldy, int N, float alpha, float beta);
    void fg_mixed_gemm_csr(const float *A, int lda, const fg_csr_matrix_t *B, float *C, int ldc, int M, int K, int N, float alpha, float beta);

    /* 线性代数扩展 */
    void fast_cholesky_s(int n, float *A, int lda);
    void fast_cholesky_d(int n, double *A, int lda);
    void fast_solve_triangular_s(bool left, bool lower, bool trans, int n, int nrhs,
                                 const float *L, int ldl, float *B, int ldb);
    void fast_solve_triangular_d(bool left, bool lower, bool trans, int n, int nrhs,
                                 const double *L, int ldl, double *B, int ldb);
    void fast_solve_cholesky_s(int n, int nrhs, const float *A, int lda, float *B, int ldb);
    void fast_solve_cholesky_d(int n, int nrhs, const double *A, int lda, double *B, int ldb);
    void fast_inverse_spd_s(int n, float *A, int lda);
    void fast_inverse_spd_d(int n, double *A, int lda);
    void fast_lu_s(int m, int n, float *A, int lda, int *ipiv);
    void fast_lu_d(int m, int n, double *A, int lda, int *ipiv);
    void fast_solve_lu_s(int n, int nrhs, const float *A, int lda, const int *ipiv, float *B, int ldb);
    void fast_solve_lu_d(int n, int nrhs, const double *A, int lda, const int *ipiv, double *B, int ldb);
    void fast_qr_s(int m, int n, float *A, int lda, float *tau);
    void fast_qr_d(int m, int n, double *A, int lda, double *tau);
    void fast_least_squares_s(int m, int n, int nrhs, float *A, int lda, float *B, int ldb);
    void fast_least_squares_d(int m, int n, int nrhs, double *A, int lda, double *B, int ldb);
    void fast_eig_sym_s(int n, float *A, int lda, float *w);
    void fast_eig_sym_d(int n, double *A, int lda, double *w);
    float fast_det_s(int n, const float *A, int lda);
    double fast_det_d(int n, const double *A, int lda);
    void fast_pca_s(int m, int n, float *A, int lda, int k, float *components, int ldd, float *var);

#ifdef __cplusplus
}
#endif

/* --------------------------------------------------------------------------
 * 实现部分 (需要定义 FAST_GEMM_IMPLEMENTATION)
 * ------------------------------------------------------------------------ */
#ifdef FAST_GEMM_IMPLEMENTATION

#include <omp.h>
#include <immintrin.h>
#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

/* ============================================================================
 * 内部函数声明 (静态内联)
 * ============================================================================ */
static void fg_kernel_6x16_scalar(int k, const float *a, const float *b, float *c, int ldc, int mr, int nr);
static void fg_kernel_4x8_scalar(int k, const double *a, const double *b, double *c, int ldc, int mr, int nr);
static void fg_pack_A_sp(int k, const float *a, int lda, float *ap, bool trans, int mr);
static void fg_pack_B_sp(int k, const float *b, int ldb, float *bp, bool trans, int nr);
static void fg_pack_A_dp(int k, const double *a, int lda, double *ap, bool trans, int mr);
static void fg_pack_B_dp(int k, const double *b, int ldb, double *bp, bool trans, int nr);
static float fg_fp16_to_fp32(fg_float16_t h);
static fg_float16_t fg_fp32_to_fp16(float f);
static float fg_bf16_to_fp32(fg_bfloat16_t b);
static fg_bfloat16_t fg_fp32_to_bf16(float f);
static void fg_sgemm_blocked(int M, int N, int K, float alpha,
                             const float *A, int lda, const float *B, int ldb,
                             float beta, float *C, int ldc,
                             bool transA, bool transB,
                             int use_avx512, int use_avx2, int use_neon);
static void fg_dgemm_blocked(int M, int N, int K, double alpha,
                             const double *A, int lda, const double *B, int ldb,
                             double beta, double *C, int ldc,
                             bool transA, bool transB,
                             int use_avx512, int use_avx2, int use_neon);
static void fg_sgemm_recursive(int M, int N, int K, float alpha,
                               const float *A, int lda, const float *B, int ldb,
                               float beta, float *C, int ldc,
                               bool transA, bool transB,
                               int use_avx512, int use_avx2, int use_neon);
static void fg_dgemm_recursive(int M, int N, int K, double alpha,
                               const double *A, int lda, const double *B, int ldb,
                               double beta, double *C, int ldc,
                               bool transA, bool transB,
                               int use_avx512, int use_avx2, int use_neon);
#if defined(__AVX512FP16__)
static void fg_hgemm_blocked(int M, int N, int K, float alpha,
                             const fg_float16_t *A, int lda, const fg_float16_t *B, int ldb,
                             float beta, float *C, int ldc,
                             bool transA, bool transB);
#endif
#if defined(__AVX512BF16__)
static void fg_bgemm_blocked(int M, int N, int K, float alpha,
                             const fg_bfloat16_t *A, int lda, const fg_bfloat16_t *B, int ldb,
                             float beta, float *C, int ldc,
                             bool transA, bool transB);
#endif
static void fg_randn(int m, int n, float *R);
static void fg_transpose(int M, int N, const float *A, int lda, float *AT, int ldat);
static int fg_randomized_svd(int M, int N, const float *A, int lda,
                             int rank, int oversample, int power_iters,
                             float **U_out, float **S_out, float **V_out);
static void fg_strassen_recursive(int n, const float *A, int lda,
                                  const float *B, int ldb,
                                  float *C, int ldc);
#define FG_CHOL_BLOCK_SIZE 128
#define FG_LU_BLOCK_SIZE 64
static void fg_tred2_s(int n, float *A, int lda, float *d, float *e);
static void fg_tql2_s(int n, float *d, float *e);
static void fg_tred2_d(int n, double *A, int lda, double *d, double *e);
static void fg_tql2_d(int n, double *d, double *e);

/* ============================================================================
 * 微内核实现 (标量、AVX2、AVX-512、NEON)
 * ============================================================================ */
static void fg_kernel_6x16_scalar(int k, const float *a, const float *b, float *c, int ldc, int mr, int nr)
{
    for (int i = 0; i < mr; ++i)
        for (int j = 0; j < nr; ++j)
        {
            float sum = 0.0f;
            for (int p = 0; p < k; ++p)
                sum += a[p * mr + i] * b[p * nr + j];
            c[i * ldc + j] += sum;
        }
}

static void fg_kernel_4x8_scalar(int k, const double *a, const double *b, double *c, int ldc, int mr, int nr)
{
    for (int i = 0; i < mr; ++i)
        for (int j = 0; j < nr; ++j)
        {
            double sum = 0.0;
            for (int p = 0; p < k; ++p)
                sum += a[p * mr + i] * b[p * nr + j];
            c[i * ldc + j] += sum;
        }
}

#if defined(__AVX2__)
static void fg_kernel_6x16_avx2(int k, const float *a, const float *b, float *c, int ldc)
{
    __m256 c00 = _mm256_setzero_ps(), c01 = _mm256_setzero_ps();
    __m256 c10 = _mm256_setzero_ps(), c11 = _mm256_setzero_ps();
    __m256 c20 = _mm256_setzero_ps(), c21 = _mm256_setzero_ps();
    __m256 c30 = _mm256_setzero_ps(), c31 = _mm256_setzero_ps();
    __m256 c40 = _mm256_setzero_ps(), c41 = _mm256_setzero_ps();
    __m256 c50 = _mm256_setzero_ps(), c51 = _mm256_setzero_ps();

    for (int p = 0; p < k; ++p)
    {
        __m256 b0 = _mm256_loadu_ps(b);
        __m256 b1 = _mm256_loadu_ps(b + 8);
        b += 16;

        __m256 a0 = _mm256_set1_ps(a[0]);
        __m256 a1 = _mm256_set1_ps(a[1]);
        __m256 a2 = _mm256_set1_ps(a[2]);
        __m256 a3 = _mm256_set1_ps(a[3]);
        __m256 a4 = _mm256_set1_ps(a[4]);
        __m256 a5 = _mm256_set1_ps(a[5]);
        a += 6;

        c00 = _mm256_fmadd_ps(a0, b0, c00);
        c01 = _mm256_fmadd_ps(a0, b1, c01);
        c10 = _mm256_fmadd_ps(a1, b0, c10);
        c11 = _mm256_fmadd_ps(a1, b1, c11);
        c20 = _mm256_fmadd_ps(a2, b0, c20);
        c21 = _mm256_fmadd_ps(a2, b1, c21);
        c30 = _mm256_fmadd_ps(a3, b0, c30);
        c31 = _mm256_fmadd_ps(a3, b1, c31);
        c40 = _mm256_fmadd_ps(a4, b0, c40);
        c41 = _mm256_fmadd_ps(a4, b1, c41);
        c50 = _mm256_fmadd_ps(a5, b0, c50);
        c51 = _mm256_fmadd_ps(a5, b1, c51);
    }

    float *r0 = c;
    float *r1 = c + ldc;
    float *r2 = c + 2 * ldc;
    float *r3 = c + 3 * ldc;
    float *r4 = c + 4 * ldc;
    float *r5 = c + 5 * ldc;

    _mm256_storeu_ps(r0, _mm256_add_ps(c00, _mm256_loadu_ps(r0)));
    _mm256_storeu_ps(r0 + 8, _mm256_add_ps(c01, _mm256_loadu_ps(r0 + 8)));
    _mm256_storeu_ps(r1, _mm256_add_ps(c10, _mm256_loadu_ps(r1)));
    _mm256_storeu_ps(r1 + 8, _mm256_add_ps(c11, _mm256_loadu_ps(r1 + 8)));
    _mm256_storeu_ps(r2, _mm256_add_ps(c20, _mm256_loadu_ps(r2)));
    _mm256_storeu_ps(r2 + 8, _mm256_add_ps(c21, _mm256_loadu_ps(r2 + 8)));
    _mm256_storeu_ps(r3, _mm256_add_ps(c30, _mm256_loadu_ps(r3)));
    _mm256_storeu_ps(r3 + 8, _mm256_add_ps(c31, _mm256_loadu_ps(r3 + 8)));
    _mm256_storeu_ps(r4, _mm256_add_ps(c40, _mm256_loadu_ps(r4)));
    _mm256_storeu_ps(r4 + 8, _mm256_add_ps(c41, _mm256_loadu_ps(r4 + 8)));
    _mm256_storeu_ps(r5, _mm256_add_ps(c50, _mm256_loadu_ps(r5)));
    _mm256_storeu_ps(r5 + 8, _mm256_add_ps(c51, _mm256_loadu_ps(r5 + 8)));
}

static void fg_kernel_4x8_avx2(int k, const double *a, const double *b, double *c, int ldc)
{
    __m256d c00 = _mm256_setzero_pd(), c01 = _mm256_setzero_pd();
    __m256d c10 = _mm256_setzero_pd(), c11 = _mm256_setzero_pd();
    __m256d c20 = _mm256_setzero_pd(), c21 = _mm256_setzero_pd();
    __m256d c30 = _mm256_setzero_pd(), c31 = _mm256_setzero_pd();

    for (int p = 0; p < k; ++p)
    {
        __m256d b0 = _mm256_loadu_pd(b);
        __m256d b1 = _mm256_loadu_pd(b + 4);
        b += 8;

        __m256d a0 = _mm256_set1_pd(a[0]);
        __m256d a1 = _mm256_set1_pd(a[1]);
        __m256d a2 = _mm256_set1_pd(a[2]);
        __m256d a3 = _mm256_set1_pd(a[3]);
        a += 4;

        c00 = _mm256_fmadd_pd(a0, b0, c00);
        c01 = _mm256_fmadd_pd(a0, b1, c01);
        c10 = _mm256_fmadd_pd(a1, b0, c10);
        c11 = _mm256_fmadd_pd(a1, b1, c11);
        c20 = _mm256_fmadd_pd(a2, b0, c20);
        c21 = _mm256_fmadd_pd(a2, b1, c21);
        c30 = _mm256_fmadd_pd(a3, b0, c30);
        c31 = _mm256_fmadd_pd(a3, b1, c31);
    }

    double *r0 = c;
    double *r1 = c + ldc;
    double *r2 = c + 2 * ldc;
    double *r3 = c + 3 * ldc;

    _mm256_storeu_pd(r0, _mm256_add_pd(c00, _mm256_loadu_pd(r0)));
    _mm256_storeu_pd(r0 + 4, _mm256_add_pd(c01, _mm256_loadu_pd(r0 + 4)));
    _mm256_storeu_pd(r1, _mm256_add_pd(c10, _mm256_loadu_pd(r1)));
    _mm256_storeu_pd(r1 + 4, _mm256_add_pd(c11, _mm256_loadu_pd(r1 + 4)));
    _mm256_storeu_pd(r2, _mm256_add_pd(c20, _mm256_loadu_pd(r2)));
    _mm256_storeu_pd(r2 + 4, _mm256_add_pd(c21, _mm256_loadu_pd(r2 + 4)));
    _mm256_storeu_pd(r3, _mm256_add_pd(c30, _mm256_loadu_pd(r3)));
    _mm256_storeu_pd(r3 + 4, _mm256_add_pd(c31, _mm256_loadu_pd(r3 + 4)));
}
#endif /* __AVX2__ */

#if defined(__ARM_NEON)
static void fg_kernel_6x16_neon(int k, const float *a, const float *b, float *c, int ldc)
{
    float32x4_t c00 = vdupq_n_f32(0), c01 = vdupq_n_f32(0), c02 = vdupq_n_f32(0), c03 = vdupq_n_f32(0);
    float32x4_t c10 = vdupq_n_f32(0), c11 = vdupq_n_f32(0), c12 = vdupq_n_f32(0), c13 = vdupq_n_f32(0);
    float32x4_t c20 = vdupq_n_f32(0), c21 = vdupq_n_f32(0), c22 = vdupq_n_f32(0), c23 = vdupq_n_f32(0);
    float32x4_t c30 = vdupq_n_f32(0), c31 = vdupq_n_f32(0), c32 = vdupq_n_f32(0), c33 = vdupq_n_f32(0);
    float32x4_t c40 = vdupq_n_f32(0), c41 = vdupq_n_f32(0), c42 = vdupq_n_f32(0), c43 = vdupq_n_f32(0);
    float32x4_t c50 = vdupq_n_f32(0), c51 = vdupq_n_f32(0), c52 = vdupq_n_f32(0), c53 = vdupq_n_f32(0);

    for (int p = 0; p < k; ++p)
    {
        float32x4_t b0 = vld1q_f32(b);
        float32x4_t b1 = vld1q_f32(b + 4);
        float32x4_t b2 = vld1q_f32(b + 8);
        float32x4_t b3 = vld1q_f32(b + 12);
        b += 16;

        float a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4], a5 = a[5];
        a += 6;

        c00 = vfmaq_n_f32(c00, b0, a0);
        c01 = vfmaq_n_f32(c01, b1, a0);
        c02 = vfmaq_n_f32(c02, b2, a0);
        c03 = vfmaq_n_f32(c03, b3, a0);
        c10 = vfmaq_n_f32(c10, b0, a1);
        c11 = vfmaq_n_f32(c11, b1, a1);
        c12 = vfmaq_n_f32(c12, b2, a1);
        c13 = vfmaq_n_f32(c13, b3, a1);
        c20 = vfmaq_n_f32(c20, b0, a2);
        c21 = vfmaq_n_f32(c21, b1, a2);
        c22 = vfmaq_n_f32(c22, b2, a2);
        c23 = vfmaq_n_f32(c23, b3, a2);
        c30 = vfmaq_n_f32(c30, b0, a3);
        c31 = vfmaq_n_f32(c31, b1, a3);
        c32 = vfmaq_n_f32(c32, b2, a3);
        c33 = vfmaq_n_f32(c33, b3, a3);
        c40 = vfmaq_n_f32(c40, b0, a4);
        c41 = vfmaq_n_f32(c41, b1, a4);
        c42 = vfmaq_n_f32(c42, b2, a4);
        c43 = vfmaq_n_f32(c43, b3, a4);
        c50 = vfmaq_n_f32(c50, b0, a5);
        c51 = vfmaq_n_f32(c51, b1, a5);
        c52 = vfmaq_n_f32(c52, b2, a5);
        c53 = vfmaq_n_f32(c53, b3, a5);
    }

#define STORE_ROW(r, v0, v1, v2, v3)                             \
    do                                                           \
    {                                                            \
        float *ptr = c + r * ldc;                                \
        vst1q_f32(ptr, vaddq_f32(v0, vld1q_f32(ptr)));           \
        vst1q_f32(ptr + 4, vaddq_f32(v1, vld1q_f32(ptr + 4)));   \
        vst1q_f32(ptr + 8, vaddq_f32(v2, vld1q_f32(ptr + 8)));   \
        vst1q_f32(ptr + 12, vaddq_f32(v3, vld1q_f32(ptr + 12))); \
    } while (0)

    STORE_ROW(0, c00, c01, c02, c03);
    STORE_ROW(1, c10, c11, c12, c13);
    STORE_ROW(2, c20, c21, c22, c23);
    STORE_ROW(3, c30, c31, c32, c33);
    STORE_ROW(4, c40, c41, c42, c43);
    STORE_ROW(5, c50, c51, c52, c53);

#undef STORE_ROW
}
#endif /* __ARM_NEON */

#if defined(__AVX512F__)
FG_INLINE void fg_kernel_6x16_avx512_asm(int k, const float *a, const float *b, float *c, int ldc)
{
    __m512 c0, c1, c2, c3, c4, c5;
    __asm__ __volatile__(
        "vxorps %%zmm0, %%zmm0, %%zmm0 \n\t"
        "vxorps %%zmm1, %%zmm1, %%zmm1 \n\t"
        "vxorps %%zmm2, %%zmm2, %%zmm2 \n\t"
        "vxorps %%zmm3, %%zmm3, %%zmm3 \n\t"
        "vxorps %%zmm4, %%zmm4, %%zmm4 \n\t"
        "vxorps %%zmm5, %%zmm5, %%zmm5 \n\t"
        "mov %[k], %%eax \n\t"
        "test %%eax, %%eax \n\t"
        "jz 2f \n\t"
        ".align 16 \n\t"
        "1: \n\t"
        "vmovups (%[b_ptr]), %%zmm6 \n\t"
        "vbroadcastss (%[a_ptr]), %%zmm7 \n\t"
        "vfmadd231ps %%zmm7, %%zmm6, %%zmm0 \n\t"
        "vbroadcastss 4(%[a_ptr]), %%zmm7 \n\t"
        "vfmadd231ps %%zmm7, %%zmm6, %%zmm1 \n\t"
        "vbroadcastss 8(%[a_ptr]), %%zmm7 \n\t"
        "vfmadd231ps %%zmm7, %%zmm6, %%zmm2 \n\t"
        "vbroadcastss 12(%[a_ptr]), %%zmm7 \n\t"
        "vfmadd231ps %%zmm7, %%zmm6, %%zmm3 \n\t"
        "vbroadcastss 16(%[a_ptr]), %%zmm7 \n\t"
        "vfmadd231ps %%zmm7, %%zmm6, %%zmm4 \n\t"
        "vbroadcastss 20(%[a_ptr]), %%zmm7 \n\t"
        "vfmadd231ps %%zmm7, %%zmm6, %%zmm5 \n\t"
        "add $64, %[b_ptr] \n\t"
        "add $24, %[a_ptr] \n\t"
        "decl %%eax \n\t"
        "jnz 1b \n\t"
        "2: \n\t"
        : "=x"(c0), "=x"(c1), "=x"(c2), "=x"(c3), "=x"(c4), "=x"(c5),
          [ a_ptr ] "+r"(a), [ b_ptr ] "+r"(b)
        : [ k ] "r"(k)
        : "eax", "zmm6", "zmm7", "memory");
    float *r0 = c;
    float *r1 = c + ldc;
    float *r2 = c + 2 * ldc;
    float *r3 = c + 3 * ldc;
    float *r4 = c + 4 * ldc;
    float *r5 = c + 5 * ldc;
    _mm512_storeu_ps(r0, _mm512_add_ps(c0, _mm512_loadu_ps(r0)));
    _mm512_storeu_ps(r1, _mm512_add_ps(c1, _mm512_loadu_ps(r1)));
    _mm512_storeu_ps(r2, _mm512_add_ps(c2, _mm512_loadu_ps(r2)));
    _mm512_storeu_ps(r3, _mm512_add_ps(c3, _mm512_loadu_ps(r3)));
    _mm512_storeu_ps(r4, _mm512_add_ps(c4, _mm512_loadu_ps(r4)));
    _mm512_storeu_ps(r5, _mm512_add_ps(c5, _mm512_loadu_ps(r5)));
}

FG_INLINE void fg_kernel_4x8_avx512_asm(int k, const double *a, const double *b, double *c, int ldc)
{
    __m512d c0, c1, c2, c3;
    __asm__ __volatile__(
        "vxorpd %%zmm0, %%zmm0, %%zmm0 \n\t"
        "vxorpd %%zmm1, %%zmm1, %%zmm1 \n\t"
        "vxorpd %%zmm2, %%zmm2, %%zmm2 \n\t"
        "vxorpd %%zmm3, %%zmm3, %%zmm3 \n\t"
        "mov %[k], %%eax \n\t"
        "test %%eax, %%eax \n\t"
        "jz 2f \n\t"
        ".align 16 \n\t"
        "1: \n\t"
        "vmovupd (%[b_ptr]), %%zmm6 \n\t"
        "vbroadcastsd (%[a_ptr]), %%zmm7 \n\t"
        "vfmadd231pd %%zmm7, %%zmm6, %%zmm0 \n\t"
        "vbroadcastsd 8(%[a_ptr]), %%zmm7 \n\t"
        "vfmadd231pd %%zmm7, %%zmm6, %%zmm1 \n\t"
        "vbroadcastsd 16(%[a_ptr]), %%zmm7 \n\t"
        "vfmadd231pd %%zmm7, %%zmm6, %%zmm2 \n\t"
        "vbroadcastsd 24(%[a_ptr]), %%zmm7 \n\t"
        "vfmadd231pd %%zmm7, %%zmm6, %%zmm3 \n\t"
        "add $64, %[b_ptr] \n\t"
        "add $32, %[a_ptr] \n\t"
        "decl %%eax \n\t"
        "jnz 1b \n\t"
        "2: \n\t"
        : "=x"(c0), "=x"(c1), "=x"(c2), "=x"(c3),
          [ a_ptr ] "+r"(a), [ b_ptr ] "+r"(b)
        : [ k ] "r"(k)
        : "eax", "zmm6", "zmm7", "memory");
    double *r0 = c;
    double *r1 = c + ldc;
    double *r2 = c + 2 * ldc;
    double *r3 = c + 3 * ldc;
    _mm512_storeu_pd(r0, _mm512_add_pd(c0, _mm512_loadu_pd(r0)));
    _mm512_storeu_pd(r1, _mm512_add_pd(c1, _mm512_loadu_pd(r1)));
    _mm512_storeu_pd(r2, _mm512_add_pd(c2, _mm512_loadu_pd(r2)));
    _mm512_storeu_pd(r3, _mm512_add_pd(c3, _mm512_loadu_pd(r3)));
}
#endif /* __AVX512F__ */

/* ----- 打包函数 ----- */
static void fg_pack_A_sp(int k, const float *a, int lda, float *ap, bool trans, int mr)
{
    if (!trans)
    {
        for (int p = 0; p < k; ++p)
            for (int i = 0; i < mr; ++i)
                ap[p * mr + i] = a[i * lda + p];
    }
    else
    {
        for (int p = 0; p < k; ++p)
            for (int i = 0; i < mr; ++i)
                ap[p * mr + i] = a[p * lda + i];
    }
}

static void fg_pack_B_sp(int k, const float *b, int ldb, float *bp, bool trans, int nr)
{
    if (!trans)
    {
        for (int p = 0; p < k; ++p)
            for (int j = 0; j < nr; ++j)
                bp[p * nr + j] = b[p * ldb + j];
    }
    else
    {
        for (int p = 0; p < k; ++p)
            for (int j = 0; j < nr; ++j)
                bp[p * nr + j] = b[j * ldb + p];
    }
}

static void fg_pack_A_dp(int k, const double *a, int lda, double *ap, bool trans, int mr)
{
    if (!trans)
    {
        for (int p = 0; p < k; ++p)
            for (int i = 0; i < mr; ++i)
                ap[p * mr + i] = a[i * lda + p];
    }
    else
    {
        for (int p = 0; p < k; ++p)
            for (int i = 0; i < mr; ++i)
                ap[p * mr + i] = a[p * lda + i];
    }
}

static void fg_pack_B_dp(int k, const double *b, int ldb, double *bp, bool trans, int nr)
{
    if (!trans)
    {
        for (int p = 0; p < k; ++p)
            for (int j = 0; j < nr; ++j)
                bp[p * nr + j] = b[p * ldb + j];
    }
    else
    {
        for (int p = 0; p < k; ++p)
            for (int j = 0; j < nr; ++j)
                bp[p * nr + j] = b[j * ldb + p];
    }
}

/* ----- FP16/BF16 转换 ----- */
static float fg_fp16_to_fp32(fg_float16_t h)
{
    union {
        uint32_t i;
        float f;
    } u;
    uint32_t sign = (h & 0x8000u) << 16;
    uint32_t exp = (h & 0x7C00u) >> 10;
    uint32_t mant = (h & 0x03FFu);
    if (exp == 0)
    {
        if (mant == 0)
            u.i = sign;
        else
        {
            exp = 1;
            while ((mant & 0x0400u) == 0)
            {
                mant <<= 1;
                exp--;
            }
            mant &= 0x03FFu;
            u.i = sign | ((exp + 112) << 23) | (mant << 13);
        }
    }
    else if (exp == 0x1F)
    {
        u.i = sign | 0x7F800000u | (mant << 13);
    }
    else
    {
        u.i = sign | ((exp + 112) << 23) | (mant << 13);
    }
    return u.f;
}

static fg_float16_t fg_fp32_to_fp16(float f)
{
    union {
        uint32_t i;
        float f;
    } u = {.f = f};
    uint32_t sign = (u.i >> 16) & 0x8000u;
    uint32_t exp = (u.i >> 23) & 0xFFu;
    uint32_t mant = (u.i >> 13) & 0x3FFu;
    if (exp == 0xFF)
    {
        return (fg_float16_t)(sign | 0x7C00u | (mant ? 0x0200u : 0));
    }
    if (exp > 112)
    {
        int diff = exp - 112;
        if (diff >= 0x1F)
            return (fg_float16_t)(sign | 0x7C00u);
        return (fg_float16_t)(sign | (diff << 10) | mant);
    }
    else if (exp >= 103)
    {
        mant |= 0x0400u;
        int shift = 113 - exp;
        mant >>= shift;
        return (fg_float16_t)(sign | mant);
    }
    return (fg_float16_t)sign;
}

static float fg_bf16_to_fp32(fg_bfloat16_t b)
{
    union {
        uint32_t i;
        float f;
    } u;
    u.i = (uint32_t)b << 16;
    return u.f;
}

static fg_bfloat16_t fg_fp32_to_bf16(float f)
{
    union {
        uint32_t i;
        float f;
    } u = {.f = f};
    uint32_t r = (u.i & 0x0000FFFFu);
    uint32_t rounding = (r > 0x8000u) || (r == 0x8000u && (u.i & 0x10000u));
    u.i = (u.i + 0x8000u + rounding) >> 16;
    return (fg_bfloat16_t)u.i;
}

/* ============================================================================
 * 分块 GEMM 实现
 * ============================================================================ */
static void fg_sgemm_blocked(int M, int N, int K, float alpha,
                             const float *A, int lda, const float *B, int ldb,
                             float beta, float *C, int ldc,
                             bool transA, bool transB,
                             int use_avx512, int use_avx2, int use_neon)
{
    int mr = FG_MR_SP, nr = FG_NR_SP, kr = FG_KR_SP;
    int mc = FG_MC_SP, nc = FG_NC_SP, kc = FG_KC_SP;

    size_t packA_size = (size_t)mr * kr * sizeof(float);
    size_t packB_size = (size_t)nr * kr * sizeof(float);
    float *pack_A = (float *)FG_ALLOCA(packA_size);
    float *pack_B = (float *)FG_ALLOCA(packB_size);

    for (int ii = 0; ii < M; ii += mc)
    {
        int mb = (ii + mc > M) ? M - ii : mc;
        for (int jj = 0; jj < N; jj += nc)
        {
            int nb = (jj + nc > N) ? N - jj : nc;
            for (int pp = 0; pp < K; pp += kc)
            {
                int kb = (pp + kc > K) ? K - pp : kc;
                for (int i = 0; i < mb; i += mr)
                {
                    int mrr = (i + mr > mb) ? mb - i : mr;
                    for (int j = 0; j < nb; j += nr)
                    {
                        int nrr = (j + nr > nb) ? nb - j : nr;

                        fg_pack_A_sp(kb, &A[(transA ? pp : ii + i) * lda + (transA ? ii + i : pp)],
                                     lda, pack_A, transA, mr);
                        fg_pack_B_sp(kb, &B[(transB ? jj + j : pp) * ldb + (transB ? pp : jj + j)],
                                     ldb, pack_B, transB, nr);

                        if (mrr == mr && nrr == nr)
                        {
#if defined(__AVX512F__)
                            if (use_avx512)
                                fg_kernel_6x16_avx512_asm(kb, pack_A, pack_B,
                                                          &C[(ii + i) * ldc + (jj + j)], ldc);
                            else
#endif
#if defined(__AVX2__)
                                if (use_avx2)
                                fg_kernel_6x16_avx2(kb, pack_A, pack_B,
                                                    &C[(ii + i) * ldc + (jj + j)], ldc);
                            else
#endif
#if defined(__ARM_NEON)
                                if (use_neon)
                                fg_kernel_6x16_neon(kb, pack_A, pack_B,
                                                    &C[(ii + i) * ldc + (jj + j)], ldc);
                            else
#endif
                                fg_kernel_6x16_scalar(kb, pack_A, pack_B,
                                                      &C[(ii + i) * ldc + (jj + j)], ldc, mrr, nrr);
                        }
                        else
                        {
                            fg_kernel_6x16_scalar(kb, pack_A, pack_B,
                                                  &C[(ii + i) * ldc + (jj + j)], ldc, mrr, nrr);
                        }
                    }
                }
            }
        }
    }
}

static void fg_dgemm_blocked(int M, int N, int K, double alpha,
                             const double *A, int lda, const double *B, int ldb,
                             double beta, double *C, int ldc,
                             bool transA, bool transB,
                             int use_avx512, int use_avx2, int use_neon)
{
    int mr = FG_MR_DP, nr = FG_NR_DP, kr = FG_KR_DP;
    int mc = FG_MC_DP, nc = FG_NC_DP, kc = FG_KC_DP;

    size_t packA_size = (size_t)mr * kr * sizeof(double);
    size_t packB_size = (size_t)nr * kr * sizeof(double);
    double *pack_A = (double *)FG_ALLOCA(packA_size);
    double *pack_B = (double *)FG_ALLOCA(packB_size);

    for (int ii = 0; ii < M; ii += mc)
    {
        int mb = (ii + mc > M) ? M - ii : mc;
        for (int jj = 0; jj < N; jj += nc)
        {
            int nb = (jj + nc > N) ? N - jj : nc;
            for (int pp = 0; pp < K; pp += kc)
            {
                int kb = (pp + kc > K) ? K - pp : kc;
                for (int i = 0; i < mb; i += mr)
                {
                    int mrr = (i + mr > mb) ? mb - i : mr;
                    for (int j = 0; j < nb; j += nr)
                    {
                        int nrr = (j + nr > nb) ? nb - j : nr;

                        fg_pack_A_dp(kb, &A[(transA ? pp : ii + i) * lda + (transA ? ii + i : pp)],
                                     lda, pack_A, transA, mr);
                        fg_pack_B_dp(kb, &B[(transB ? jj + j : pp) * ldb + (transB ? pp : jj + j)],
                                     ldb, pack_B, transB, nr);

                        if (mrr == mr && nrr == nr)
                        {
#if defined(__AVX512F__)
                            if (use_avx512)
                                fg_kernel_4x8_avx512_asm(kb, pack_A, pack_B,
                                                         &C[(ii + i) * ldc + (jj + j)], ldc);
                            else
#endif
#if defined(__AVX2__)
                                if (use_avx2)
                                fg_kernel_4x8_avx2(kb, pack_A, pack_B,
                                                   &C[(ii + i) * ldc + (jj + j)], ldc);
                            else
#endif
                                fg_kernel_4x8_scalar(kb, pack_A, pack_B,
                                                     &C[(ii + i) * ldc + (jj + j)], ldc, mrr, nrr);
                        }
                        else
                        {
                            fg_kernel_4x8_scalar(kb, pack_A, pack_B,
                                                 &C[(ii + i) * ldc + (jj + j)], ldc, mrr, nrr);
                        }
                    }
                }
            }
        }
    }
}

/* 递归分块 (Cache‑Oblivious) */
static void fg_sgemm_recursive(int M, int N, int K, float alpha,
                               const float *A, int lda, const float *B, int ldb,
                               float beta, float *C, int ldc,
                               bool transA, bool transB,
                               int use_avx512, int use_avx2, int use_neon)
{
    if (M <= FG_MC_SP && N <= FG_NC_SP && K <= FG_KC_SP)
    {
        fg_sgemm_blocked(M, N, K, alpha, A, lda, B, ldb, beta, C, ldc,
                         transA, transB, use_avx512, use_avx2, use_neon);
        return;
    }
    if (M >= N && M >= K)
    {
        int M1 = M / 2;
        int M2 = M - M1;
        fg_sgemm_recursive(M1, N, K, alpha, A, lda, B, ldb, beta, C, ldc,
                           transA, transB, use_avx512, use_avx2, use_neon);
        fg_sgemm_recursive(M2, N, K, alpha,
                           A + (transA ? 0 : M1 * lda), lda,
                           B, ldb, beta,
                           C + M1 * ldc, ldc,
                           transA, transB, use_avx512, use_avx2, use_neon);
    }
    else if (N >= M && N >= K)
    {
        int N1 = N / 2;
        int N2 = N - N1;
        fg_sgemm_recursive(M, N1, K, alpha, A, lda, B, ldb, beta, C, ldc,
                           transA, transB, use_avx512, use_avx2, use_neon);
        fg_sgemm_recursive(M, N2, K, alpha, A, lda,
                           B + (transB ? 0 : N1), ldb, beta,
                           C + N1, ldc,
                           transA, transB, use_avx512, use_avx2, use_neon);
    }
    else
    {
        int K1 = K / 2;
        int K2 = K - K1;
        fg_sgemm_recursive(M, N, K1, alpha, A, lda, B, ldb, 1.0f, C, ldc,
                           transA, transB, use_avx512, use_avx2, use_neon);
        fg_sgemm_recursive(M, N, K2, alpha,
                           A + (transA ? 0 : K1), lda,
                           B + (transB ? K1 : 0), ldb,
                           1.0f, C, ldc,
                           transA, transB, use_avx512, use_avx2, use_neon);
    }
}

static void fg_dgemm_recursive(int M, int N, int K, double alpha,
                               const double *A, int lda, const double *B, int ldb,
                               double beta, double *C, int ldc,
                               bool transA, bool transB,
                               int use_avx512, int use_avx2, int use_neon)
{
    if (M <= FG_MC_DP && N <= FG_NC_DP && K <= FG_KC_DP)
    {
        fg_dgemm_blocked(M, N, K, alpha, A, lda, B, ldb, beta, C, ldc,
                         transA, transB, use_avx512, use_avx2, use_neon);
        return;
    }
    if (M >= N && M >= K)
    {
        int M1 = M / 2;
        int M2 = M - M1;
        fg_dgemm_recursive(M1, N, K, alpha, A, lda, B, ldb, beta, C, ldc,
                           transA, transB, use_avx512, use_avx2, use_neon);
        fg_dgemm_recursive(M2, N, K, alpha, A + M1 * lda, lda, B, ldb, beta,
                           C + M1 * ldc, ldc,
                           transA, transB, use_avx512, use_avx2, use_neon);
    }
    else if (N >= M && N >= K)
    {
        int N1 = N / 2;
        int N2 = N - N1;
        fg_dgemm_recursive(M, N1, K, alpha, A, lda, B, ldb, beta, C, ldc,
                           transA, transB, use_avx512, use_avx2, use_neon);
        fg_dgemm_recursive(M, N2, K, alpha, A, lda, B + N1, ldb, beta,
                           C + N1, ldc,
                           transA, transB, use_avx512, use_avx2, use_neon);
    }
    else
    {
        int K1 = K / 2;
        int K2 = K - K1;
        fg_dgemm_recursive(M, N, K1, alpha, A, lda, B, ldb, 1.0, C, ldc,
                           transA, transB, use_avx512, use_avx2, use_neon);
        fg_dgemm_recursive(M, N, K2, alpha,
                           A + (transA ? 0 : K1), lda,
                           B + (transB ? K1 : 0), ldb,
                           1.0, C, ldc,
                           transA, transB, use_avx512, use_avx2, use_neon);
    }
}

/* ============================================================================
 * 公共 GEMM 接口
 * ============================================================================ */
void fast_sgemm(bool transA, bool transB, int M, int N, int K, float alpha,
                const float *A, int lda, const float *B, int ldb, float beta, float *C, int ldc)
{
    fg_auto_tune_params();

    if (alpha == 0.0f)
    {
        if (beta != 1.0f)
        {
            for (int i = 0; i < M; ++i)
                for (int j = 0; j < N; ++j)
                    C[i * ldc + j] *= beta;
        }
        return;
    }
    if (beta != 1.0f)
    {
        for (int i = 0; i < M; ++i)
            for (int j = 0; j < N; ++j)
                C[i * ldc + j] *= beta;
    }

    int use_avx512 = 0, use_avx2 = 0, use_neon = 0;
#if defined(__AVX512F__)
    use_avx512 = FG_CPU_AVX512();
#endif
#if defined(__AVX2__)
    if (!use_avx512)
        use_avx2 = FG_CPU_AVX2();
#endif
#if defined(__ARM_NEON)
    use_neon = FG_CPU_NEON();
#endif

    if (M > FG_MC_SP * 2 && N > FG_NC_SP * 2 && K > FG_KC_SP * 2)
    {
        fg_sgemm_recursive(M, N, K, alpha, A, lda, B, ldb, beta, C, ldc,
                           transA, transB, use_avx512, use_avx2, use_neon);
    }
    else
    {
        fg_sgemm_blocked(M, N, K, alpha, A, lda, B, ldb, beta, C, ldc,
                         transA, transB, use_avx512, use_avx2, use_neon);
    }
}

void fast_dgemm(bool transA, bool transB, int M, int N, int K, double alpha,
                const double *A, int lda, const double *B, int ldb, double beta, double *C, int ldc)
{
    fg_auto_tune_params();

    if (alpha == 0.0)
    {
        if (beta != 1.0)
        {
            for (int i = 0; i < M; ++i)
                for (int j = 0; j < N; ++j)
                    C[i * ldc + j] *= beta;
        }
        return;
    }
    if (beta != 1.0)
    {
        for (int i = 0; i < M; ++i)
            for (int j = 0; j < N; ++j)
                C[i * ldc + j] *= beta;
    }

    int use_avx512 = 0, use_avx2 = 0, use_neon = 0;
#if defined(__AVX512F__)
    use_avx512 = FG_CPU_AVX512();
#endif
#if defined(__AVX2__)
    if (!use_avx512)
        use_avx2 = FG_CPU_AVX2();
#endif
#if defined(__ARM_NEON)
    use_neon = FG_CPU_NEON();
#endif

    if (M > FG_MC_DP * 2 && N > FG_NC_DP * 2 && K > FG_KC_DP * 2)
    {
        fg_dgemm_recursive(M, N, K, alpha, A, lda, B, ldb, beta, C, ldc,
                           transA, transB, use_avx512, use_avx2, use_neon);
    }
    else
    {
        fg_dgemm_blocked(M, N, K, alpha, A, lda, B, ldb, beta, C, ldc,
                         transA, transB, use_avx512, use_avx2, use_neon);
    }
}

/* ============================================================================
 * FP16 / BF16 GEMM (硬件加速 + 软件回退)
 * ============================================================================ */
#if defined(__AVX512FP16__)
static void fg_hgemm_blocked(int M, int N, int K, float alpha,
                             const fg_float16_t *A, int lda, const fg_float16_t *B, int ldb,
                             float beta, float *C, int ldc,
                             bool transA, bool transB)
{
    int mr = FG_MR_SP, nr = FG_NR_SP, kr = FG_KR_SP;
    size_t packA_size = (size_t)mr * kr * sizeof(fg_float16_t);
    size_t packB_size = (size_t)nr * kr * sizeof(fg_float16_t);
    fg_float16_t *pack_A = (fg_float16_t *)FG_ALLOCA(packA_size);
    fg_float16_t *pack_B = (fg_float16_t *)FG_ALLOCA(packB_size);

    for (int i = 0; i < M; i += mr)
    {
        int mb = (i + mr > M) ? M - i : mr;
        for (int j = 0; j < N; j += nr)
        {
            int nb = (j + nr > N) ? N - j : nr;
            for (int p = 0; p < K; p += kr)
            {
                int kb = (p + kr > K) ? K - p : kr;

                for (int pp = 0; pp < kb; ++pp)
                {
                    for (int ii = 0; ii < mr; ++ii)
                    {
                        pack_A[pp * mr + ii] = A[(transA ? p + pp : i + ii) * lda + (transA ? i + ii : p + pp)];
                    }
                }
                for (int pp = 0; pp < kb; ++pp)
                {
                    for (int jj = 0; jj < nr; ++jj)
                    {
                        pack_B[pp * nr + jj] = B[(transB ? j + jj : p + pp) * ldb + (transB ? p + pp : j + jj)];
                    }
                }

                if (mb == mr && nb == nr)
                {
                    __m512 c0 = _mm512_setzero_ps(), c1 = _mm512_setzero_ps();
                    __m512 c2 = _mm512_setzero_ps(), c3 = _mm512_setzero_ps();
                    __m512 c4 = _mm512_setzero_ps(), c5 = _mm512_setzero_ps();
                    for (int pp = 0; pp < kb; ++pp)
                    {
                        __m256i b_i16 = _mm256_loadu_si256((const __m256i *)(pack_B + pp * nr));
                        __m512 b0 = _mm512_cvtph_ps(b_i16);
                        fg_float16_t a0v = pack_A[pp * mr + 0];
                        fg_float16_t a1v = pack_A[pp * mr + 1];
                        fg_float16_t a2v = pack_A[pp * mr + 2];
                        fg_float16_t a3v = pack_A[pp * mr + 3];
                        fg_float16_t a4v = pack_A[pp * mr + 4];
                        fg_float16_t a5v = pack_A[pp * mr + 5];
                        __m512 a0 = _mm512_set1_ps(fg_fp16_to_fp32(a0v));
                        __m512 a1 = _mm512_set1_ps(fg_fp16_to_fp32(a1v));
                        __m512 a2 = _mm512_set1_ps(fg_fp16_to_fp32(a2v));
                        __m512 a3 = _mm512_set1_ps(fg_fp16_to_fp32(a3v));
                        __m512 a4 = _mm512_set1_ps(fg_fp16_to_fp32(a4v));
                        __m512 a5 = _mm512_set1_ps(fg_fp16_to_fp32(a5v));
                        c0 = _mm512_fmadd_ps(a0, b0, c0);
                        c1 = _mm512_fmadd_ps(a1, b0, c1);
                        c2 = _mm512_fmadd_ps(a2, b0, c2);
                        c3 = _mm512_fmadd_ps(a3, b0, c3);
                        c4 = _mm512_fmadd_ps(a4, b0, c4);
                        c5 = _mm512_fmadd_ps(a5, b0, c5);
                    }
                    float *r0 = C + i * ldc + j;
                    float *r1 = r0 + ldc;
                    float *r2 = r0 + 2 * ldc;
                    float *r3 = r0 + 3 * ldc;
                    float *r4 = r0 + 4 * ldc;
                    float *r5 = r0 + 5 * ldc;
                    _mm512_storeu_ps(r0, _mm512_add_ps(c0, _mm512_loadu_ps(r0)));
                    _mm512_storeu_ps(r1, _mm512_add_ps(c1, _mm512_loadu_ps(r1)));
                    _mm512_storeu_ps(r2, _mm512_add_ps(c2, _mm512_loadu_ps(r2)));
                    _mm512_storeu_ps(r3, _mm512_add_ps(c3, _mm512_loadu_ps(r3)));
                    _mm512_storeu_ps(r4, _mm512_add_ps(c4, _mm512_loadu_ps(r4)));
                    _mm512_storeu_ps(r5, _mm512_add_ps(c5, _mm512_loadu_ps(r5)));
                }
                else
                {
                    for (int ii = 0; ii < mb; ++ii)
                    {
                        for (int jj = 0; jj < nb; ++jj)
                        {
                            float sum = 0.0f;
                            for (int pp = 0; pp < kb; ++pp)
                            {
                                sum += fg_fp16_to_fp32(pack_A[pp * mr + ii]) *
                                       fg_fp16_to_fp32(pack_B[pp * nr + jj]);
                            }
                            C[(i + ii) * ldc + (j + jj)] += alpha * sum;
                        }
                    }
                }
            }
        }
    }
}
#endif

void fast_hgemm(bool transA, bool transB, int M, int N, int K, float alpha,
                const fg_float16_t *A, int lda, const fg_float16_t *B, int ldb,
                float beta, float *C, int ldc)
{
    if (beta != 1.0f)
    {
        for (int i = 0; i < M; ++i)
            for (int j = 0; j < N; ++j)
                C[i * ldc + j] *= beta;
    }
#if defined(__AVX512FP16__)
    if (FG_CPU_AVX512FP16())
    {
        fg_hgemm_blocked(M, N, K, alpha, A, lda, B, ldb, beta, C, ldc, transA, transB);
        return;
    }
#endif
    float *Af = (float *)malloc((size_t)M * K * sizeof(float));
    float *Bf = (float *)malloc((size_t)K * N * sizeof(float));
    if (!Af || !Bf)
    {
        if (Af)
            free(Af);
        if (Bf)
            free(Bf);
        return;
    }
    for (int i = 0; i < M * K; ++i)
        Af[i] = fg_fp16_to_fp32(A[i]);
    for (int i = 0; i < K * N; ++i)
        Bf[i] = fg_fp16_to_fp32(B[i]);
    fast_sgemm(transA, transB, M, N, K, alpha, Af, lda, Bf, ldb, beta, C, ldc);
    free(Af);
    free(Bf);
}

#if defined(__AVX512BF16__)
static void fg_bgemm_blocked(int M, int N, int K, float alpha,
                             const fg_bfloat16_t *A, int lda, const fg_bfloat16_t *B, int ldb,
                             float beta, float *C, int ldc,
                             bool transA, bool transB)
{
    int mr = FG_MR_SP, nr = FG_NR_SP, kr = FG_KR_SP;
    size_t packA_size = (size_t)mr * kr * sizeof(fg_bfloat16_t);
    size_t packB_size = (size_t)nr * kr * sizeof(fg_bfloat16_t);
    fg_bfloat16_t *pack_A = (fg_bfloat16_t *)FG_ALLOCA(packA_size);
    fg_bfloat16_t *pack_B = (fg_bfloat16_t *)FG_ALLOCA(packB_size);

    for (int i = 0; i < M; i += mr)
    {
        int mb = (i + mr > M) ? M - i : mr;
        for (int j = 0; j < N; j += nr)
        {
            int nb = (j + nr > N) ? N - j : nr;
            for (int p = 0; p < K; p += kr)
            {
                int kb = (p + kr > K) ? K - p : kr;

                for (int pp = 0; pp < kb; ++pp)
                {
                    for (int ii = 0; ii < mr; ++ii)
                    {
                        pack_A[pp * mr + ii] = fg_fp32_to_bf16(
                            fg_bf16_to_fp32(A[(transA ? p + pp : i + ii) * lda + (transA ? i + ii : p + pp)]));
                    }
                    for (int jj = 0; jj < nr; ++jj)
                    {
                        pack_B[pp * nr + jj] = fg_fp32_to_bf16(
                            fg_bf16_to_fp32(B[(transB ? j + jj : p + pp) * ldb + (transB ? p + pp : j + jj)]));
                    }
                }

                if (mb == mr && nb == nr)
                {
                    __m512 c0 = _mm512_setzero_ps(), c1 = _mm512_setzero_ps();
                    __m512 c2 = _mm512_setzero_ps(), c3 = _mm512_setzero_ps();
                    __m512 c4 = _mm512_setzero_ps(), c5 = _mm512_setzero_ps();
                    for (int pp = 0; pp < kb; ++pp)
                    {
                        __m256i b_bf16 = _mm256_loadu_si256((const __m256i *)(pack_B + pp * nr));
                        __m512 b0 = _mm512_cvtnebf16_ps(b_bf16);
                        fg_bfloat16_t a0v = pack_A[pp * mr + 0];
                        fg_bfloat16_t a1v = pack_A[pp * mr + 1];
                        fg_bfloat16_t a2v = pack_A[pp * mr + 2];
                        fg_bfloat16_t a3v = pack_A[pp * mr + 3];
                        fg_bfloat16_t a4v = pack_A[pp * mr + 4];
                        fg_bfloat16_t a5v = pack_A[pp * mr + 5];
                        __m512 a0 = _mm512_set1_ps(fg_bf16_to_fp32(a0v));
                        __m512 a1 = _mm512_set1_ps(fg_bf16_to_fp32(a1v));
                        __m512 a2 = _mm512_set1_ps(fg_bf16_to_fp32(a2v));
                        __m512 a3 = _mm512_set1_ps(fg_bf16_to_fp32(a3v));
                        __m512 a4 = _mm512_set1_ps(fg_bf16_to_fp32(a4v));
                        __m512 a5 = _mm512_set1_ps(fg_bf16_to_fp32(a5v));
                        c0 = _mm512_fmadd_ps(a0, b0, c0);
                        c1 = _mm512_fmadd_ps(a1, b0, c1);
                        c2 = _mm512_fmadd_ps(a2, b0, c2);
                        c3 = _mm512_fmadd_ps(a3, b0, c3);
                        c4 = _mm512_fmadd_ps(a4, b0, c4);
                        c5 = _mm512_fmadd_ps(a5, b0, c5);
                    }
                    float *r0 = C + i * ldc + j;
                    float *r1 = r0 + ldc;
                    float *r2 = r0 + 2 * ldc;
                    float *r3 = r0 + 3 * ldc;
                    float *r4 = r0 + 4 * ldc;
                    float *r5 = r0 + 5 * ldc;
                    _mm512_storeu_ps(r0, _mm512_add_ps(c0, _mm512_loadu_ps(r0)));
                    _mm512_storeu_ps(r1, _mm512_add_ps(c1, _mm512_loadu_ps(r1)));
                    _mm512_storeu_ps(r2, _mm512_add_ps(c2, _mm512_loadu_ps(r2)));
                    _mm512_storeu_ps(r3, _mm512_add_ps(c3, _mm512_loadu_ps(r3)));
                    _mm512_storeu_ps(r4, _mm512_add_ps(c4, _mm512_loadu_ps(r4)));
                    _mm512_storeu_ps(r5, _mm512_add_ps(c5, _mm512_loadu_ps(r5)));
                }
                else
                {
                    for (int ii = 0; ii < mb; ++ii)
                    {
                        for (int jj = 0; jj < nb; ++jj)
                        {
                            float sum = 0.0f;
                            for (int pp = 0; pp < kb; ++pp)
                            {
                                sum += fg_bf16_to_fp32(pack_A[pp * mr + ii]) *
                                       fg_bf16_to_fp32(pack_B[pp * nr + jj]);
                            }
                            C[(i + ii) * ldc + (j + jj)] += alpha * sum;
                        }
                    }
                }
            }
        }
    }
}
#endif

void fast_bgemm(bool transA, bool transB, int M, int N, int K, float alpha,
                const fg_bfloat16_t *A, int lda, const fg_bfloat16_t *B, int ldb,
                float beta, float *C, int ldc)
{
    if (beta != 1.0f)
    {
        for (int i = 0; i < M; ++i)
            for (int j = 0; j < N; ++j)
                C[i * ldc + j] *= beta;
    }
#if defined(__AVX512BF16__)
    if (FG_CPU_AVX512BF16())
    {
        fg_bgemm_blocked(M, N, K, alpha, A, lda, B, ldb, beta, C, ldc, transA, transB);
        return;
    }
#endif
    float *Af = (float *)malloc((size_t)M * K * sizeof(float));
    float *Bf = (float *)malloc((size_t)K * N * sizeof(float));
    if (!Af || !Bf)
    {
        if (Af)
            free(Af);
        if (Bf)
            free(Bf);
        return;
    }
    for (int i = 0; i < M * K; ++i)
        Af[i] = fg_bf16_to_fp32(A[i]);
    for (int i = 0; i < K * N; ++i)
        Bf[i] = fg_bf16_to_fp32(B[i]);
    fast_sgemm(transA, transB, M, N, K, alpha, Af, lda, Bf, ldb, beta, C, ldc);
    free(Af);
    free(Bf);
}

/* ============================================================================
 * 低秩近似 (随机化 SVD)
 * ============================================================================ */
static void fg_randn(int m, int n, float *R)
{
    static uint32_t x = 123456789, y = 362436069, z = 521288629, w = 88675123;
    for (int i = 0; i < m * n; ++i)
    {
        uint32_t t = x ^ (x << 11);
        x = y;
        y = z;
        z = w;
        w = w ^ (w >> 19) ^ t ^ (t >> 8);
        float u1 = (float)w / (float)UINT32_MAX;
        t = x ^ (x << 11);
        x = y;
        y = z;
        z = w;
        w = w ^ (w >> 19) ^ t ^ (t >> 8);
        float u2 = (float)w / (float)UINT32_MAX;
        R[i] = sqrtf(-2.0f * logf(u1 + 1e-10f)) * cosf(2.0f * 3.14159265358979323846f * u2);
    }
}

static void fg_transpose(int M, int N, const float *A, int lda, float *AT, int ldat)
{
    for (int i = 0; i < M; ++i)
        for (int j = 0; j < N; ++j)
            AT[j * ldat + i] = A[i * lda + j];
}

static int fg_randomized_svd(int M, int N, const float *A, int lda,
                             int rank, int oversample, int power_iters,
                             float **U_out, float **S_out, float **V_out)
{
    int r = rank + oversample;
    if (r > M || r > N)
        r = (M < N) ? M : N;
    float *Omega = (float *)malloc((size_t)N * r * sizeof(float));
    fg_randn(N, r, Omega);
    float *Y = (float *)malloc((size_t)M * r * sizeof(float));
    fast_sgemm(false, false, M, r, N, 1.0f, A, lda, Omega, N, 0.0f, Y, M);
    float *Z = (float *)malloc((size_t)N * r * sizeof(float));
    for (int iter = 0; iter < power_iters; ++iter)
    {
        fast_sgemm(true, false, N, r, M, 1.0f, A, lda, Y, M, 0.0f, Z, N);
        fast_sgemm(false, false, M, r, N, 1.0f, A, lda, Z, N, 0.0f, Y, M);
    }
    float *Q = (float *)malloc((size_t)M * r * sizeof(float));
    memcpy(Q, Y, (size_t)M * r * sizeof(float));
    for (int j = 0; j < r; ++j)
    {
        float norm = 0.0f;
        for (int i = 0; i < M; ++i)
            norm += Q[i * r + j] * Q[i * r + j];
        norm = sqrtf(norm);
        for (int i = 0; i < M; ++i)
            Q[i * r + j] /= norm;
        for (int k = j + 1; k < r; ++k)
        {
            float dot = 0.0f;
            for (int i = 0; i < M; ++i)
                dot += Q[i * r + j] * Q[i * r + k];
            for (int i = 0; i < M; ++i)
                Q[i * r + k] -= dot * Q[i * r + j];
        }
    }
    float *B = (float *)malloc((size_t)r * N * sizeof(float));
    fast_sgemm(true, false, r, N, M, 1.0f, Q, M, A, lda, 0.0f, B, r);
    float *Vt = (float *)malloc((size_t)N * r * sizeof(float));
    fg_transpose(r, N, B, r, Vt, N);
    for (int j = 0; j < r; ++j)
    {
        float norm = 0.0f;
        for (int i = 0; i < N; ++i)
            norm += Vt[i * r + j] * Vt[i * r + j];
        norm = sqrtf(norm);
        for (int i = 0; i < N; ++i)
            Vt[i * r + j] /= norm;
        for (int k = j + 1; k < r; ++k)
        {
            float dot = 0.0f;
            for (int i = 0; i < N; ++i)
                dot += Vt[i * r + j] * Vt[i * r + k];
            for (int i = 0; i < N; ++i)
                Vt[i * r + k] -= dot * Vt[i * r + j];
        }
    }
    *U_out = Q;
    *V_out = Vt;
    *S_out = NULL;
    free(Omega);
    free(Y);
    free(Z);
    free(B);
    return r;
}

void fast_lowrank_gemm(bool transA, bool transB, int M, int N, int K, float alpha,
                       const float *A, int lda, const float *B, int ldb,
                       float beta, float *C, int ldc, int rank)
{
    if (rank <= 0 || rank >= K)
    {
        fast_sgemm(transA, transB, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc);
        return;
    }
    float *U, *S, *V;
    int r = fg_randomized_svd(K, N, B, ldb, rank, 5, 2, &U, &S, &V);
    float *T = (float *)malloc((size_t)M * r * sizeof(float));
    fast_sgemm(transA, false, M, r, K, 1.0f, A, lda, U, K, 0.0f, T, M);
    fast_sgemm(false, true, M, N, r, alpha, T, M, V, N, beta, C, ldc);
    free(T);
    free(U);
    free(V);
    if (S)
        free(S);
}

/* ============================================================================
 * Strassen 算法 (方阵, 2的幂)
 * ============================================================================ */
static void fg_strassen_recursive(int n, const float *A, int lda,
                                  const float *B, int ldb,
                                  float *C, int ldc)
{
    if (n <= 128)
    {
        fast_sgemm(false, false, n, n, n, 1.0f, A, lda, B, ldb, 0.0f, C, ldc);
        return;
    }
    int n2 = n / 2;
    const float *A11 = A, *A12 = A + n2;
    const float *A21 = A + n2 * lda, *A22 = A + n2 * lda + n2;
    const float *B11 = B, *B12 = B + n2;
    const float *B21 = B + n2 * ldb, *B22 = B + n2 * ldb + n2;
    float *C11 = C, *C12 = C + n2;
    float *C21 = C + n2 * ldc, *C22 = C + n2 * ldc + n2;

    size_t sz = (size_t)n2 * n2 * sizeof(float);
    float *M1 = (float *)malloc(sz), *M2 = (float *)malloc(sz), *M3 = (float *)malloc(sz);
    float *M4 = (float *)malloc(sz), *M5 = (float *)malloc(sz), *M6 = (float *)malloc(sz);
    float *M7 = (float *)malloc(sz), *T1 = (float *)malloc(sz), *T2 = (float *)malloc(sz);
    if (!M1 || !M2 || !M3 || !M4 || !M5 || !M6 || !M7 || !T1 || !T2)
    {
        fast_sgemm(false, false, n, n, n, 1.0f, A, lda, B, ldb, 0.0f, C, ldc);
        goto cleanup;
    }

    for (int i = 0; i < n2; ++i)
    {
        for (int j = 0; j < n2; ++j)
        {
            T1[i * n2 + j] = A11[i * lda + j] + A22[i * lda + j];
            T2[i * n2 + j] = B11[i * ldb + j] + B22[i * ldb + j];
        }
    }
    fg_strassen_recursive(n2, T1, n2, T2, n2, M1, n2);
    for (int i = 0; i < n2; ++i)
        for (int j = 0; j < n2; ++j)
            T1[i * n2 + j] = A21[i * lda + j] + A22[i * lda + j];
    fg_strassen_recursive(n2, T1, n2, B11, n2, M2, n2);
    for (int i = 0; i < n2; ++i)
        for (int j = 0; j < n2; ++j)
            T1[i * n2 + j] = B12[i * ldb + j] - B22[i * ldb + j];
    fg_strassen_recursive(n2, A11, n2, T1, n2, M3, n2);
    for (int i = 0; i < n2; ++i)
        for (int j = 0; j < n2; ++j)
            T1[i * n2 + j] = B21[i * ldb + j] - B11[i * ldb + j];
    fg_strassen_recursive(n2, A22, n2, T1, n2, M4, n2);
    for (int i = 0; i < n2; ++i)
        for (int j = 0; j < n2; ++j)
            T1[i * n2 + j] = A11[i * lda + j] + A12[i * lda + j];
    fg_strassen_recursive(n2, T1, n2, B22, n2, M5, n2);
    for (int i = 0; i < n2; ++i)
    {
        for (int j = 0; j < n2; ++j)
        {
            T1[i * n2 + j] = A21[i * lda + j] - A11[i * lda + j];
            T2[i * n2 + j] = B11[i * ldb + j] + B12[i * ldb + j];
        }
    }
    fg_strassen_recursive(n2, T1, n2, T2, n2, M6, n2);
    for (int i = 0; i < n2; ++i)
    {
        for (int j = 0; j < n2; ++j)
        {
            T1[i * n2 + j] = A12[i * lda + j] - A22[i * lda + j];
            T2[i * n2 + j] = B21[i * ldb + j] + B22[i * ldb + j];
        }
    }
    fg_strassen_recursive(n2, T1, n2, T2, n2, M7, n2);

    for (int i = 0; i < n2; ++i)
    {
        for (int j = 0; j < n2; ++j)
        {
            C11[i * ldc + j] = M1[i * n2 + j] + M4[i * n2 + j] - M5[i * n2 + j] + M7[i * n2 + j];
            C12[i * ldc + j] = M3[i * n2 + j] + M5[i * n2 + j];
            C21[i * ldc + j] = M2[i * n2 + j] + M4[i * n2 + j];
            C22[i * ldc + j] = M1[i * n2 + j] - M2[i * n2 + j] + M3[i * n2 + j] + M6[i * n2 + j];
        }
    }

cleanup:
    free(M1);
    free(M2);
    free(M3);
    free(M4);
    free(M5);
    free(M6);
    free(M7);
    free(T1);
    free(T2);
}

void fast_strassen_gemm(int n, float alpha, const float *A, int lda,
                        const float *B, int ldb, float beta, float *C, int ldc)
{
    if (alpha == 1.0f && beta == 0.0f && (n & (n - 1)) == 0)
    {
        fg_strassen_recursive(n, A, lda, B, ldb, C, ldc);
    }
    else
    {
        fast_sgemm(false, false, n, n, n, alpha, A, lda, B, ldb, beta, C, ldc);
    }
}

/* ============================================================================
 * AMX BF16 内核 (x86 AMX)
 * ============================================================================ */
#if defined(__AMX_BF16__) || defined(__AMX_INT8__)
#include <x86gprintrin.h>
static __thread int g_amx_tile_configured = 0;
static void fg_amx_configure_tiles(void)
{
    if (g_amx_tile_configured)
        return;
    __tilecfg tileconfig = {0};
    tileconfig.palette_id = 1;
    tileconfig.start_row = 0;
    tileconfig.rows[0] = 16;
    tileconfig.cols[0] = 64;
    tileconfig.rows[1] = 16;
    tileconfig.cols[1] = 64;
    tileconfig.rows[2] = 16;
    tileconfig.cols[2] = 64;
    _tile_loadconfig(&tileconfig);
    g_amx_tile_configured = 1;
}
static void fg_amx_bf16_kernel(int M, int N, int K,
                               const fg_bfloat16_t *A, int lda,
                               const fg_bfloat16_t *B, int ldb,
                               float *C, int ldc,
                               float alpha, float beta)
{
    const int tm = 16, tn = 16, tk = 32;
    fg_amx_configure_tiles();
    if (beta != 1.0f)
    {
        for (int i = 0; i < M; ++i)
            for (int j = 0; j < N; ++j)
                C[i * ldc + j] *= beta;
    }
    for (int i = 0; i < M; i += tm)
    {
        int mb = (i + tm > M) ? M - i : tm;
        for (int j = 0; j < N; j += tn)
        {
            _tile_zero(2);
            for (int p = 0; p < K; p += tk)
            {
                _tile_loadd(0, A + i * lda + p, lda * sizeof(fg_bfloat16_t));
                _tile_loadd(1, B + p * ldb + j, ldb * sizeof(fg_bfloat16_t));
                _tile_dpbf16ps(2, 0, 1);
            }
            _tile_stored(2, C + i * ldc + j, ldc * sizeof(float));
            if (alpha != 1.0f)
            {
                for (int ii = 0; ii < mb; ++ii)
                    for (int jj = 0; jj < tn; ++jj)
                        C[(i + ii) * ldc + (j + jj)] *= alpha;
            }
        }
    }
}
void fast_amx_bf16_gemm(bool transA, bool transB, int M, int N, int K, float alpha,
                        const fg_bfloat16_t *A, int lda, const fg_bfloat16_t *B, int ldb,
                        float beta, float *C, int ldc)
{
    if (transA || transB)
    {
        fast_bgemm(transA, transB, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc);
        return;
    }
    if (!FG_CPU_AMX_BF16())
    {
        fast_bgemm(false, false, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc);
        return;
    }
    fg_amx_bf16_kernel(M, N, K, A, lda, B, ldb, C, ldc, alpha, beta);
}
#endif /* __AMX_BF16__ */

/* ============================================================================
 * 稀疏矩阵 CSR
 * ============================================================================ */
void fg_spmv_csr(const fg_csr_matrix_t *A, const float *x, float *y, float alpha, float beta)
{
    for (int i = 0; i < A->n_rows; ++i)
    {
        float sum = 0.0f;
        for (int j = A->row_ptr[i]; j < A->row_ptr[i + 1]; ++j)
        {
            sum += A->values[j] * x[A->col_idx[j]];
        }
        y[i] = alpha * sum + beta * y[i];
    }
}

void fg_spmm_csr(const fg_csr_matrix_t *A, const float *B, int ldb, float *Y, int ldy, int N, float alpha, float beta)
{
    for (int i = 0; i < A->n_rows; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            float sum = 0.0f;
            for (int p = A->row_ptr[i]; p < A->row_ptr[i + 1]; ++p)
            {
                sum += A->values[p] * B[A->col_idx[p] * ldb + j];
            }
            Y[i * ldy + j] = alpha * sum + beta * Y[i * ldy + j];
        }
    }
}

void fg_mixed_gemm_csr(const float *A, int lda, const fg_csr_matrix_t *B, float *C, int ldc, int M, int K, int N, float alpha, float beta)
{
    for (int j = 0; j < N; ++j)
    {
        float *bj = (float *)calloc((size_t)K, sizeof(float));
        for (int row = 0; row < B->n_rows; ++row)
        {
            for (int p = B->row_ptr[row]; p < B->row_ptr[row + 1]; ++p)
            {
                if (B->col_idx[p] == j)
                {
                    bj[row] = B->values[p];
                    break;
                }
            }
        }
        for (int i = 0; i < M; ++i)
        {
            float sum = 0.0f;
            for (int p = 0; p < K; ++p)
                sum += A[i * lda + p] * bj[p];
            C[i * ldc + j] = alpha * sum + beta * C[i * ldc + j];
        }
        free(bj);
    }
}

/* ============================================================================
 * 批量 GEMM
 * ============================================================================ */
void fast_sgemm_batched(int batch_count, bool transA, bool transB, int M, int N, int K, float alpha,
                        const float *const *A, int lda, const float *const *B, int ldb,
                        float beta, float *const *C, int ldc)
{
#pragma omp parallel for schedule(dynamic) if (batch_count > 4)
    for (int b = 0; b < batch_count; ++b)
    {
        fast_sgemm(transA, transB, M, N, K, alpha, A[b], lda, B[b], ldb, beta, C[b], ldc);
    }
}

void fast_dgemm_batched(int batch_count, bool transA, bool transB, int M, int N, int K, double alpha,
                        const double *const *A, int lda, const double *const *B, int ldb,
                        double beta, double *const *C, int ldc)
{
#pragma omp parallel for schedule(dynamic) if (batch_count > 4)
    for (int b = 0; b < batch_count; ++b)
    {
        fast_dgemm(transA, transB, M, N, K, alpha, A[b], lda, B[b], ldb, beta, C[b], ldc);
    }
}

/* ============================================================================
 * 线性代数扩展
 * ============================================================================ */
/* ----- Cholesky 分解 ----- */
void fast_cholesky_s(int n, float *A, int lda)
{
    for (int jb = 0; jb < n; jb += FG_CHOL_BLOCK_SIZE)
    {
        int jb_end = (jb + FG_CHOL_BLOCK_SIZE < n) ? jb + FG_CHOL_BLOCK_SIZE : n;
        int jb_size = jb_end - jb;
        for (int j = jb; j < jb_end; ++j)
        {
            for (int i = jb; i < j; ++i)
            {
                float sum = A[j * lda + i];
                for (int k = jb; k < i; ++k)
                    sum -= A[i * lda + k] * A[j * lda + k];
                A[j * lda + i] = sum / A[i * lda + i];
            }
            float sum = A[j * lda + j];
            for (int k = jb; k < j; ++k)
                sum -= A[j * lda + k] * A[j * lda + k];
            A[j * lda + j] = sqrtf(sum);
        }
        if (jb_end < n)
        {
            for (int i = jb_end; i < n; ++i)
            {
                for (int j = jb; j < jb_end; ++j)
                {
                    float sum = A[i * lda + j];
                    for (int k = jb; k < j; ++k)
                        sum -= A[j * lda + k] * A[i * lda + k];
                    A[i * lda + j] = sum / A[j * lda + j];
                }
            }
            fast_sgemm(false, true, n - jb_end, n - jb_end, jb_size,
                       -1.0f, &A[jb_end * lda + jb], lda, &A[jb_end * lda + jb], lda,
                       1.0f, &A[jb_end * lda + jb_end], lda);
        }
    }
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            A[i * lda + j] = 0.0f;
}

void fast_cholesky_d(int n, double *A, int lda)
{
    for (int jb = 0; jb < n; jb += FG_CHOL_BLOCK_SIZE)
    {
        int jb_end = (jb + FG_CHOL_BLOCK_SIZE < n) ? jb + FG_CHOL_BLOCK_SIZE : n;
        int jb_size = jb_end - jb;
        for (int j = jb; j < jb_end; ++j)
        {
            for (int i = jb; i < j; ++i)
            {
                double sum = A[j * lda + i];
                for (int k = jb; k < i; ++k)
                    sum -= A[i * lda + k] * A[j * lda + k];
                A[j * lda + i] = sum / A[i * lda + i];
            }
            double sum = A[j * lda + j];
            for (int k = jb; k < j; ++k)
                sum -= A[j * lda + k] * A[j * lda + k];
            A[j * lda + j] = sqrt(sum);
        }
        if (jb_end < n)
        {
            for (int i = jb_end; i < n; ++i)
            {
                for (int j = jb; j < jb_end; ++j)
                {
                    double sum = A[i * lda + j];
                    for (int k = jb; k < j; ++k)
                        sum -= A[j * lda + k] * A[i * lda + k];
                    A[i * lda + j] = sum / A[j * lda + j];
                }
            }
            fast_dgemm(false, true, n - jb_end, n - jb_end, jb_size,
                       -1.0, &A[jb_end * lda + jb], lda, &A[jb_end * lda + jb], lda,
                       1.0, &A[jb_end * lda + jb_end], lda);
        }
    }
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            A[i * lda + j] = 0.0;
}

/* ----- 三角求解 (左除，下三角) ----- */
static void fg_trsm_left_lower_notrans_s(int n, int nrhs, const float *L, int ldl, float *B, int ldb)
{
    for (int j = 0; j < nrhs; ++j)
    {
        for (int i = 0; i < n; ++i)
        {
            float sum = B[i * ldb + j];
            for (int k = 0; k < i; ++k)
                sum -= L[i * ldl + k] * B[k * ldb + j];
            B[i * ldb + j] = sum / L[i * ldl + i];
        }
    }
}
static void fg_trsm_left_lower_trans_s(int n, int nrhs, const float *L, int ldl, float *B, int ldb)
{
    for (int i = 0; i < n; ++i)
    {
        float inv = 1.0f / L[i * ldl + i];
        for (int j = 0; j < nrhs; ++j)
        {
            float sum = B[i * ldb + j];
            for (int k = 0; k < i; ++k)
                sum -= L[k * ldl + i] * B[k * ldb + j];
            B[i * ldb + j] = sum * inv;
        }
    }
}
static void fg_trsm_left_upper_notrans_s(int n, int nrhs, const float *U, int ldu, float *B, int ldb)
{
    for (int j = 0; j < nrhs; ++j)
    {
        for (int i = n - 1; i >= 0; --i)
        {
            float sum = B[i * ldb + j];
            for (int k = i + 1; k < n; ++k)
                sum -= U[i * ldu + k] * B[k * ldb + j];
            B[i * ldb + j] = sum / U[i * ldu + i];
        }
    }
}
static void fg_trsm_left_lower_notrans_d(int n, int nrhs, const double *L, int ldl, double *B, int ldb)
{
    for (int j = 0; j < nrhs; ++j)
    {
        for (int i = 0; i < n; ++i)
        {
            double sum = B[i * ldb + j];
            for (int k = 0; k < i; ++k)
                sum -= L[i * ldl + k] * B[k * ldb + j];
            B[i * ldb + j] = sum / L[i * ldl + i];
        }
    }
}
static void fg_trsm_left_lower_trans_d(int n, int nrhs, const double *L, int ldl, double *B, int ldb)
{
    for (int i = 0; i < n; ++i)
    {
        double inv = 1.0 / L[i * ldl + i];
        for (int j = 0; j < nrhs; ++j)
        {
            double sum = B[i * ldb + j];
            for (int k = 0; k < i; ++k)
                sum -= L[k * ldl + i] * B[k * ldb + j];
            B[i * ldb + j] = sum * inv;
        }
    }
}
static void fg_trsm_left_upper_notrans_d(int n, int nrhs, const double *U, int ldu, double *B, int ldb)
{
    for (int j = 0; j < nrhs; ++j)
    {
        for (int i = n - 1; i >= 0; --i)
        {
            double sum = B[i * ldb + j];
            for (int k = i + 1; k < n; ++k)
                sum -= U[i * ldu + k] * B[k * ldb + j];
            B[i * ldb + j] = sum / U[i * ldu + i];
        }
    }
}

void fast_solve_triangular_s(bool left, bool lower, bool trans, int n, int nrhs,
                             const float *L, int ldl, float *B, int ldb)
{
    if (left && lower)
    {
        if (trans)
            fg_trsm_left_lower_trans_s(n, nrhs, L, ldl, B, ldb);
        else
            fg_trsm_left_lower_notrans_s(n, nrhs, L, ldl, B, ldb);
    }
    else if (left && !lower)
    {
        fg_trsm_left_upper_notrans_s(n, nrhs, L, ldl, B, ldb);
    }
}
void fast_solve_triangular_d(bool left, bool lower, bool trans, int n, int nrhs,
                             const double *L, int ldl, double *B, int ldb)
{
    if (left && lower)
    {
        if (trans)
            fg_trsm_left_lower_trans_d(n, nrhs, L, ldl, B, ldb);
        else
            fg_trsm_left_lower_notrans_d(n, nrhs, L, ldl, B, ldb);
    }
    else if (left && !lower)
    {
        fg_trsm_left_upper_notrans_d(n, nrhs, L, ldl, B, ldb);
    }
}

void fast_solve_cholesky_s(int n, int nrhs, const float *A, int lda, float *B, int ldb)
{
    float *L = (float *)malloc((size_t)n * lda * sizeof(float));
    memcpy(L, A, (size_t)n * lda * sizeof(float));
    fast_cholesky_s(n, L, lda);
    fast_solve_triangular_s(true, true, false, n, nrhs, L, lda, B, ldb);
    fast_solve_triangular_s(true, true, true, n, nrhs, L, lda, B, ldb);
    free(L);
}
void fast_solve_cholesky_d(int n, int nrhs, const double *A, int lda, double *B, int ldb)
{
    double *L = (double *)malloc((size_t)n * lda * sizeof(double));
    memcpy(L, A, (size_t)n * lda * sizeof(double));
    fast_cholesky_d(n, L, lda);
    fast_solve_triangular_d(true, true, false, n, nrhs, L, lda, B, ldb);
    fast_solve_triangular_d(true, true, true, n, nrhs, L, lda, B, ldb);
    free(L);
}

void fast_inverse_spd_s(int n, float *A, int lda)
{
    fast_cholesky_s(n, A, lda);
    for (int i = 0; i < n; ++i)
    {
        A[i * lda + i] = 1.0f / A[i * lda + i];
        for (int j = 0; j < i; ++j)
        {
            float sum = 0.0f;
            for (int k = j; k < i; ++k)
                sum -= A[i * lda + k] * A[k * lda + j];
            A[i * lda + j] = sum / A[i * lda + i];
        }
    }
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j <= i; ++j)
        {
            float sum = 0.0f;
            for (int k = i; k < n; ++k)
                sum += A[k * lda + i] * A[k * lda + j];
            A[i * lda + j] = A[j * lda + i] = sum;
        }
    }
}
void fast_inverse_spd_d(int n, double *A, int lda)
{
    fast_cholesky_d(n, A, lda);
    for (int i = 0; i < n; ++i)
    {
        A[i * lda + i] = 1.0 / A[i * lda + i];
        for (int j = 0; j < i; ++j)
        {
            double sum = 0.0;
            for (int k = j; k < i; ++k)
                sum -= A[i * lda + k] * A[k * lda + j];
            A[i * lda + j] = sum / A[i * lda + i];
        }
    }
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j <= i; ++j)
        {
            double sum = 0.0;
            for (int k = i; k < n; ++k)
                sum += A[k * lda + i] * A[k * lda + j];
            A[i * lda + j] = A[j * lda + i] = sum;
        }
    }
}

/* ----- LU 分解 ----- */
void fast_lu_s(int m, int n, float *A, int lda, int *ipiv)
{
    int mn = (m < n) ? m : n;
    for (int jb = 0; jb < mn; jb += FG_LU_BLOCK_SIZE)
    {
        int jb_end = (jb + FG_LU_BLOCK_SIZE < mn) ? jb + FG_LU_BLOCK_SIZE : mn;
        for (int j = jb; j < jb_end; ++j)
        {
            int piv = j;
            float max_val = fabsf(A[j * lda + j]);
            for (int i = j + 1; i < m; ++i)
            {
                if (fabsf(A[i * lda + j]) > max_val)
                {
                    max_val = fabsf(A[i * lda + j]);
                    piv = i;
                }
            }
            if (piv != j)
            {
                for (int k = 0; k < n; ++k)
                {
                    float t = A[j * lda + k];
                    A[j * lda + k] = A[piv * lda + k];
                    A[piv * lda + k] = t;
                }
            }
            ipiv[j] = piv;
            float inv = 1.0f / A[j * lda + j];
            for (int i = j + 1; i < m; ++i)
                A[i * lda + j] *= inv;
            for (int i = j + 1; i < m; ++i)
                for (int k = j + 1; k < jb_end; ++k)
                    A[i * lda + k] -= A[i * lda + j] * A[j * lda + k];
        }
        if (jb_end < n)
        {
            for (int j = jb; j < jb_end; ++j)
            {
                int piv = ipiv[j];
                if (piv != j)
                {
                    for (int k = jb_end; k < n; ++k)
                    {
                        float t = A[j * lda + k];
                        A[j * lda + k] = A[piv * lda + k];
                        A[piv * lda + k] = t;
                    }
                }
            }
            fast_sgemm(false, false, m - jb, n - jb_end, jb_end - jb,
                       -1.0f, &A[jb * lda + jb], lda, &A[jb * lda + jb_end], lda,
                       1.0f, &A[jb * lda + jb_end], lda);
        }
    }
}
void fast_lu_d(int m, int n, double *A, int lda, int *ipiv)
{
    int mn = (m < n) ? m : n;
    for (int jb = 0; jb < mn; jb += FG_LU_BLOCK_SIZE)
    {
        int jb_end = (jb + FG_LU_BLOCK_SIZE < mn) ? jb + FG_LU_BLOCK_SIZE : mn;
        for (int j = jb; j < jb_end; ++j)
        {
            int piv = j;
            double max_val = fabs(A[j * lda + j]);
            for (int i = j + 1; i < m; ++i)
            {
                if (fabs(A[i * lda + j]) > max_val)
                {
                    max_val = fabs(A[i * lda + j]);
                    piv = i;
                }
            }
            if (piv != j)
            {
                for (int k = 0; k < n; ++k)
                {
                    double t = A[j * lda + k];
                    A[j * lda + k] = A[piv * lda + k];
                    A[piv * lda + k] = t;
                }
            }
            ipiv[j] = piv;
            double inv = 1.0 / A[j * lda + j];
            for (int i = j + 1; i < m; ++i)
                A[i * lda + j] *= inv;
            for (int i = j + 1; i < m; ++i)
                for (int k = j + 1; k < jb_end; ++k)
                    A[i * lda + k] -= A[i * lda + j] * A[j * lda + k];
        }
        if (jb_end < n)
        {
            for (int j = jb; j < jb_end; ++j)
            {
                int piv = ipiv[j];
                if (piv != j)
                {
                    for (int k = jb_end; k < n; ++k)
                    {
                        double t = A[j * lda + k];
                        A[j * lda + k] = A[piv * lda + k];
                        A[piv * lda + k] = t;
                    }
                }
            }
            fast_dgemm(false, false, m - jb, n - jb_end, jb_end - jb,
                       -1.0, &A[jb * lda + jb], lda, &A[jb * lda + jb_end], lda,
                       1.0, &A[jb * lda + jb_end], lda);
        }
    }
}
void fast_solve_lu_s(int n, int nrhs, const float *A, int lda, const int *ipiv, float *B, int ldb)
{
    float *X = (float *)malloc((size_t)n * ldb * sizeof(float));
    memcpy(X, B, (size_t)n * ldb * sizeof(float));
    for (int i = 0; i < n; ++i)
    {
        int piv = ipiv[i];
        if (piv != i)
        {
            for (int j = 0; j < nrhs; ++j)
            {
                float t = X[i * ldb + j];
                X[i * ldb + j] = X[piv * ldb + j];
                X[piv * ldb + j] = t;
            }
        }
        for (int j = i + 1; j < n; ++j)
        {
            float f = A[j * lda + i];
            for (int k = 0; k < nrhs; ++k)
                X[j * ldb + k] -= f * X[i * ldb + k];
        }
    }
    for (int i = n - 1; i >= 0; --i)
    {
        float inv = 1.0f / A[i * lda + i];
        for (int k = 0; k < nrhs; ++k)
        {
            float sum = X[i * ldb + k];
            for (int j = i + 1; j < n; ++j)
                sum -= A[i * lda + j] * X[j * ldb + k];
            X[i * ldb + k] = sum * inv;
        }
    }
    memcpy(B, X, (size_t)n * ldb * sizeof(float));
    free(X);
}
void fast_solve_lu_d(int n, int nrhs, const double *A, int lda, const int *ipiv, double *B, int ldb)
{
    double *X = (double *)malloc((size_t)n * ldb * sizeof(double));
    memcpy(X, B, (size_t)n * ldb * sizeof(double));
    for (int i = 0; i < n; ++i)
    {
        int piv = ipiv[i];
        if (piv != i)
        {
            for (int j = 0; j < nrhs; ++j)
            {
                double t = X[i * ldb + j];
                X[i * ldb + j] = X[piv * ldb + j];
                X[piv * ldb + j] = t;
            }
        }
        for (int j = i + 1; j < n; ++j)
        {
            double f = A[j * lda + i];
            for (int k = 0; k < nrhs; ++k)
                X[j * ldb + k] -= f * X[i * ldb + k];
        }
    }
    for (int i = n - 1; i >= 0; --i)
    {
        double inv = 1.0 / A[i * lda + i];
        for (int k = 0; k < nrhs; ++k)
        {
            double sum = X[i * ldb + k];
            for (int j = i + 1; j < n; ++j)
                sum -= A[i * lda + j] * X[j * ldb + k];
            X[i * ldb + k] = sum * inv;
        }
    }
    memcpy(B, X, (size_t)n * ldb * sizeof(double));
    free(X);
}

/* ----- QR 分解 (Householder) ----- */
void fast_qr_s(int m, int n, float *A, int lda, float *tau)
{
    for (int j = 0; j < n; ++j)
    {
        float normx = 0.0f;
        for (int i = j; i < m; ++i)
            normx += A[i * lda + j] * A[i * lda + j];
        normx = sqrtf(normx);
        if (normx > 0.0f)
        {
            float alpha = A[j * lda + j];
            float beta = -copysignf(normx, alpha);
            float v1 = alpha - beta;
            tau[j] = -v1 / beta;
            A[j * lda + j] = beta;
            for (int i = j + 1; i < m; ++i)
                A[i * lda + j] /= v1;
            for (int k = j + 1; k < n; ++k)
            {
                float dot = 0.0f;
                for (int i = j; i < m; ++i)
                    dot += A[i * lda + j] * A[i * lda + k];
                dot *= tau[j];
                for (int i = j; i < m; ++i)
                    A[i * lda + k] -= dot * A[i * lda + j];
            }
        }
        else
        {
            tau[j] = 0.0f;
        }
    }
}
void fast_qr_d(int m, int n, double *A, int lda, double *tau)
{
    for (int j = 0; j < n; ++j)
    {
        double normx = 0.0;
        for (int i = j; i < m; ++i)
            normx += A[i * lda + j] * A[i * lda + j];
        normx = sqrt(normx);
        if (normx > 0.0)
        {
            double alpha = A[j * lda + j];
            double beta = -copysign(normx, alpha);
            double v1 = alpha - beta;
            tau[j] = -v1 / beta;
            A[j * lda + j] = beta;
            for (int i = j + 1; i < m; ++i)
                A[i * lda + j] /= v1;
            for (int k = j + 1; k < n; ++k)
            {
                double dot = 0.0;
                for (int i = j; i < m; ++i)
                    dot += A[i * lda + j] * A[i * lda + k];
                dot *= tau[j];
                for (int i = j; i < m; ++i)
                    A[i * lda + k] -= dot * A[i * lda + j];
            }
        }
        else
        {
            tau[j] = 0.0;
        }
    }
}
void fast_least_squares_s(int m, int n, int nrhs, float *A, int lda, float *B, int ldb)
{
    float *tau = (float *)malloc((size_t)n * sizeof(float));
    fast_qr_s(m, n, A, lda, tau);
    for (int j = 0; j < n; ++j)
    {
        if (tau[j] != 0.0f)
        {
            for (int k = 0; k < nrhs; ++k)
            {
                float dot = 0.0f;
                for (int i = j; i < m; ++i)
                    dot += A[i * lda + j] * B[i * ldb + k];
                dot *= tau[j];
                for (int i = j; i < m; ++i)
                    B[i * ldb + k] -= dot * A[i * lda + j];
            }
        }
    }
    for (int k = 0; k < nrhs; ++k)
    {
        for (int i = n - 1; i >= 0; --i)
        {
            float sum = B[i * ldb + k];
            for (int j = i + 1; j < n; ++j)
                sum -= A[i * lda + j] * B[j * ldb + k];
            B[i * ldb + k] = sum / A[i * lda + i];
        }
    }
    free(tau);
}
void fast_least_squares_d(int m, int n, int nrhs, double *A, int lda, double *B, int ldb)
{
    double *tau = (double *)malloc((size_t)n * sizeof(double));
    fast_qr_d(m, n, A, lda, tau);
    for (int j = 0; j < n; ++j)
    {
        if (tau[j] != 0.0)
        {
            for (int k = 0; k < nrhs; ++k)
            {
                double dot = 0.0;
                for (int i = j; i < m; ++i)
                    dot += A[i * lda + j] * B[i * ldb + k];
                dot *= tau[j];
                for (int i = j; i < m; ++i)
                    B[i * ldb + k] -= dot * A[i * lda + j];
            }
        }
    }
    for (int k = 0; k < nrhs; ++k)
    {
        for (int i = n - 1; i >= 0; --i)
        {
            double sum = B[i * ldb + k];
            for (int j = i + 1; j < n; ++j)
                sum -= A[i * lda + j] * B[j * ldb + k];
            B[i * ldb + k] = sum / A[i * lda + i];
        }
    }
    free(tau);
}

/* ----- 对称特征值 (三对角化 + 隐式QL) ----- */
static void fg_tred2_s(int n, float *A, int lda, float *d, float *e)
{
    for (int i = n - 1; i > 0; --i)
    {
        float scale = 0.0f, h = 0.0f;
        for (int k = 0; k < i; ++k)
            scale += fabsf(A[i * lda + k]);
        if (scale == 0.0f)
        {
            e[i] = A[i * lda + i - 1];
            continue;
        }
        for (int k = 0; k < i; ++k)
        {
            A[i * lda + k] /= scale;
            h += A[i * lda + k] * A[i * lda + k];
        }
        float f = A[i * lda + i - 1];
        float g = (f >= 0) ? -sqrtf(h) : sqrtf(h);
        e[i] = scale * g;
        h -= f * g;
        A[i * lda + i - 1] = f - g;
        f = 0.0f;
        for (int j = 0; j < i; ++j)
        {
            A[j * lda + i] = A[i * lda + j] / h;
            float sum = 0.0f;
            for (int k = 0; k <= j; ++k)
                sum += A[j * lda + k] * A[i * lda + k];
            for (int k = j + 1; k < i; ++k)
                sum += A[k * lda + j] * A[i * lda + k];
            e[j] = sum / h;
            f += e[j] * A[i * lda + j];
        }
        float hh = f / (h + h);
        for (int j = 0; j < i; ++j)
        {
            f = A[i * lda + j];
            e[j] = g = e[j] - hh * f;
            for (int k = 0; k <= j; ++k)
                A[j * lda + k] -= f * e[k] + g * A[i * lda + k];
        }
        for (int k = 0; k < i; ++k)
            A[i * lda + k] *= scale;
        d[i] = h;
    }
    d[0] = 0.0f;
    e[0] = 0.0f;
    for (int i = 0; i < n; ++i)
        d[i] = A[i * lda + i];
}
static void fg_tql2_s(int n, float *d, float *e)
{
    for (int i = 1; i < n; ++i)
        e[i - 1] = e[i];
    e[n - 1] = 0.0f;
    for (int l = 0; l < n; ++l)
    {
        int iter = 0, m;
        do
        {
            for (m = l; m < n - 1; ++m)
                if (fabsf(e[m]) <= 1e-10f)
                    break;
            if (m != l)
            {
                float g = (d[l + 1] - d[l]) / (2.0f * e[l]);
                float r = sqrtf(g * g + 1.0f);
                g = d[m] - d[l] + e[l] / (g + (g >= 0 ? r : -r));
                float s = 1.0f, c = 1.0f, p = 0.0f;
                for (int i = m - 1; i >= l; --i)
                {
                    float f = s * e[i];
                    float b = c * e[i];
                    if (fabsf(f) >= fabsf(g))
                    {
                        c = g / f;
                        r = sqrtf(c * c + 1.0f);
                        e[i + 1] = f * r;
                        s = 1.0f / r;
                        c *= s;
                    }
                    else
                    {
                        s = f / g;
                        r = sqrtf(s * s + 1.0f);
                        e[i + 1] = g * r;
                        c = 1.0f / r;
                        s *= c;
                    }
                    g = d[i + 1] - p;
                    r = (d[i] - g) * s + 2.0f * c * b;
                    p = s * r;
                    d[i + 1] = g + p;
                    g = c * r - b;
                }
                d[l] -= p;
                e[l] = g;
                e[m] = 0.0f;
            }
        } while (m != l && ++iter < 30);
    }
}
void fast_eig_sym_s(int n, float *A, int lda, float *w)
{
    float *e = (float *)malloc((size_t)n * sizeof(float));
    fg_tred2_s(n, A, lda, w, e);
    fg_tql2_s(n, w, e);
    free(e);
}
static void fg_tred2_d(int n, double *A, int lda, double *d, double *e)
{
    for (int i = n - 1; i > 0; --i)
    {
        double scale = 0.0, h = 0.0;
        for (int k = 0; k < i; ++k)
            scale += fabs(A[i * lda + k]);
        if (scale == 0.0)
        {
            e[i] = A[i * lda + i - 1];
            continue;
        }
        for (int k = 0; k < i; ++k)
        {
            A[i * lda + k] /= scale;
            h += A[i * lda + k] * A[i * lda + k];
        }
        double f = A[i * lda + i - 1];
        double g = (f >= 0) ? -sqrt(h) : sqrt(h);
        e[i] = scale * g;
        h -= f * g;
        A[i * lda + i - 1] = f - g;
        f = 0.0;
        for (int j = 0; j < i; ++j)
        {
            A[j * lda + i] = A[i * lda + j] / h;
            double sum = 0.0;
            for (int k = 0; k <= j; ++k)
                sum += A[j * lda + k] * A[i * lda + k];
            for (int k = j + 1; k < i; ++k)
                sum += A[k * lda + j] * A[i * lda + k];
            e[j] = sum / h;
            f += e[j] * A[i * lda + j];
        }
        double hh = f / (h + h);
        for (int j = 0; j < i; ++j)
        {
            f = A[i * lda + j];
            e[j] = g = e[j] - hh * f;
            for (int k = 0; k <= j; ++k)
                A[j * lda + k] -= f * e[k] + g * A[i * lda + k];
        }
        for (int k = 0; k < i; ++k)
            A[i * lda + k] *= scale;
        d[i] = h;
    }
    d[0] = 0.0;
    e[0] = 0.0;
    for (int i = 0; i < n; ++i)
        d[i] = A[i * lda + i];
}
static void fg_tql2_d(int n, double *d, double *e)
{
    for (int i = 1; i < n; ++i)
        e[i - 1] = e[i];
    e[n - 1] = 0.0;
    for (int l = 0; l < n; ++l)
    {
        int iter = 0, m;
        do
        {
            for (m = l; m < n - 1; ++m)
                if (fabs(e[m]) <= 1e-12)
                    break;
            if (m != l)
            {
                double g = (d[l + 1] - d[l]) / (2.0 * e[l]);
                double r = sqrt(g * g + 1.0);
                g = d[m] - d[l] + e[l] / (g + (g >= 0 ? r : -r));
                double s = 1.0, c = 1.0, p = 0.0;
                for (int i = m - 1; i >= l; --i)
                {
                    double f = s * e[i];
                    double b = c * e[i];
                    if (fabs(f) >= fabs(g))
                    {
                        c = g / f;
                        r = sqrt(c * c + 1.0);
                        e[i + 1] = f * r;
                        s = 1.0 / r;
                        c *= s;
                    }
                    else
                    {
                        s = f / g;
                        r = sqrt(s * s + 1.0);
                        e[i + 1] = g * r;
                        c = 1.0 / r;
                        s *= c;
                    }
                    g = d[i + 1] - p;
                    r = (d[i] - g) * s + 2.0 * c * b;
                    p = s * r;
                    d[i + 1] = g + p;
                    g = c * r - b;
                }
                d[l] -= p;
                e[l] = g;
                e[m] = 0.0;
            }
        } while (m != l && ++iter < 30);
    }
}
void fast_eig_sym_d(int n, double *A, int lda, double *w)
{
    double *e = (double *)malloc((size_t)n * sizeof(double));
    fg_tred2_d(n, A, lda, w, e);
    fg_tql2_d(n, w, e);
    free(e);
}

/* ----- 行列式 ----- */
float fast_det_s(int n, const float *A, int lda)
{
    float *Ac = (float *)malloc((size_t)n * lda * sizeof(float));
    memcpy(Ac, A, (size_t)n * lda * sizeof(float));
    int *ipiv = (int *)malloc((size_t)n * sizeof(int));
    fast_lu_s(n, n, Ac, lda, ipiv);
    float det = 1.0f;
    int sign = 1;
    for (int i = 0; i < n; ++i)
    {
        det *= Ac[i * lda + i];
        if (ipiv[i] != i)
            sign = -sign;
    }
    free(Ac);
    free(ipiv);
    return sign * det;
}
double fast_det_d(int n, const double *A, int lda)
{
    double *Ac = (double *)malloc((size_t)n * lda * sizeof(double));
    memcpy(Ac, A, (size_t)n * lda * sizeof(double));
    int *ipiv = (int *)malloc((size_t)n * sizeof(int));
    fast_lu_d(n, n, Ac, lda, ipiv);
    double det = 1.0;
    int sign = 1;
    for (int i = 0; i < n; ++i)
    {
        det *= Ac[i * lda + i];
        if (ipiv[i] != i)
            sign = -sign;
    }
    free(Ac);
    free(ipiv);
    return sign * det;
}

/* ----- PCA (基于随机 SVD) ----- */
void fast_pca_s(int m, int n, float *A, int lda, int k, float *components, int ldd, float *var)
{
    float *U, *S, *V;
    int r = fg_randomized_svd(m, n, A, lda, k, 5, 2, &U, &S, &V);
    for (int i = 0; i < k; ++i)
        for (int j = 0; j < n; ++j)
            components[i * ldd + j] = V[j * r + i];
    if (var)
    {
        // 方差占比可选实现
    }
    free(U);
    free(V);
}

#endif /* FAST_GEMM_IMPLEMENTATION */
#endif /* FAST_GEMM_H */