/**
 * @file rtmpbc_simd.h
 * @brief High-performance SIMD intrinsics wrapper (AVX2/FMA, ARM NEON, and Scalar fallback).
 *
 * Provides vectorized Fused Multiply-Add (FMA) and dot-product kernels
 * for sub-10 nanosecond real-time execution.
 */

#ifndef RTMPBC_SIMD_H
#define RTMPBC_SIMD_H

#include "rtmpbc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Detect AVX2 / FMA on x86_64 */
#if defined(__AVX2__) && defined(__FMA__)
#include <immintrin.h>
#define RTMPBC_HAS_AVX2 1
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define RTMPBC_HAS_NEON 1
#else
#define RTMPBC_HAS_SCALAR 1
#endif

/**
 * @brief Vectorized dot product of two arrays: result = sum(a[i] * b[i]).
 * Arrays must be 64-byte cache-line aligned.
 */
static inline rtmpbc_real_t rtmpbc_simd_dot(const rtmpbc_real_t* a, const rtmpbc_real_t* b, size_t len) {
    rtmpbc_real_t total = 0.0;
    size_t i = 0;

#if defined(RTMPBC_HAS_AVX2) && !defined(RTMPBC_USE_FLOAT)
    __m256d acc = _mm256_setzero_pd();
    for (; i + 4 <= len; i += 4) {
        __m256d va = _mm256_load_pd(&a[i]);
        __m256d vb = _mm256_load_pd(&b[i]);
        acc = _mm256_fmadd_pd(va, vb, acc);
    }
    /* Horizontal add */
    __m128d hi = _mm256_extractf128_pd(acc, 1);
    __m128d lo = _mm256_castpd256_pd128(acc);
    __m128d sum2 = _mm_add_pd(hi, lo);
    __m128d high = _mm_unpackhi_pd(sum2, sum2);
    __m128d final_sum = _mm_add_sd(sum2, high);
    total = _mm_cvtsd_f64(final_sum);

#elif defined(RTMPBC_HAS_NEON) && !defined(RTMPBC_USE_FLOAT) && defined(__aarch64__)
    float64x2_t acc = vdupq_n_f64(0.0);
    for (; i + 2 <= len; i += 2) {
        float64x2_t va = vld1q_f64(&a[i]);
        float64x2_t vb = vld1q_f64(&b[i]);
        acc = vfmaq_f64(acc, va, vb);
    }
    total = vgetq_lane_f64(acc, 0) + vgetq_lane_f64(acc, 1);
#endif

    /* Scalar cleanup loop */
    for (; i < len; ++i) {
        total += a[i] * b[i];
    }

    return total;
}

/**
 * @brief Vectorized FMA accumulation: dest[i] += a[i] * b[i]
 */
static inline void rtmpbc_simd_fma(rtmpbc_real_t* dest, const rtmpbc_real_t* a, const rtmpbc_real_t* b, size_t len) {
    size_t i = 0;

#if defined(RTMPBC_HAS_AVX2) && !defined(RTMPBC_USE_FLOAT)
    for (; i + 4 <= len; i += 4) {
        __m256d vd = _mm256_load_pd(&dest[i]);
        __m256d va = _mm256_load_pd(&a[i]);
        __m256d vb = _mm256_load_pd(&b[i]);
        vd = _mm256_fmadd_pd(va, vb, vd);
        _mm256_store_pd(&dest[i], vd);
    }
#elif defined(RTMPBC_HAS_NEON) && !defined(RTMPBC_USE_FLOAT) && defined(__aarch64__)
    for (; i + 2 <= len; i += 2) {
        float64x2_t vd = vld1q_f64(&dest[i]);
        float64x2_t va = vld1q_f64(&a[i]);
        float64x2_t vb = vld1q_f64(&b[i]);
        vd = vfmaq_f64(vd, va, vb);
        vst1q_f64(&dest[i], vd);
    }
#endif

    for (; i < len; ++i) {
        dest[i] += a[i] * b[i];
    }
}

#ifdef __cplusplus
}
#endif

#endif /* RTMPBC_SIMD_H */
