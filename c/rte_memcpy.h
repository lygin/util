#ifndef _RTE_MEMCPY_H_
#define _RTE_MEMCPY_H_
#include <stdlib.h>
#include <stdint.h>
#include <emmintrin.h>
#define __rte_always_inline inline __attribute__((always_inline))

static inline void rte_mov16(uint8_t *dst, const uint8_t *src)
{
	__m128i xmm0;

	xmm0 = _mm_loadu_si128((const __m128i *)src);
	_mm_storeu_si128((__m128i *)dst, xmm0);
}

/**
 * Copy 32 bytes from one location to another,
 * locations should not overlap.
 */
static __rte_always_inline void
rte_mov32(uint8_t *dst, const uint8_t *src)
{
	__m256i ymm0;

	ymm0 = _mm256_loadu_si256((const __m256i *)src);
	_mm256_storeu_si256((__m256i *)dst, ymm0);
}
/**
 * Copy 64 bytes from one location to another,
 * locations should not overlap.
 */
static __rte_always_inline void rte_mov64(uint8_t *dst, const uint8_t *src)
{
	
	__m512i zmm0;

	zmm0 = _mm512_loadu_si512((const void *)src);
	_mm512_storeu_si512((void *)dst, zmm0);
}
#endif