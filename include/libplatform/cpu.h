#pragma once

#include <stdint.h>

enum {
  LIBPLATFORM_CPU_FEATURE_SSE2 = 1u << 0,
  LIBPLATFORM_CPU_FEATURE_SSSE3 = 1u << 1,
  LIBPLATFORM_CPU_FEATURE_AVX2 = 1u << 2,
  LIBPLATFORM_CPU_FEATURE_NEON = 1u << 3,
  LIBPLATFORM_CPU_FEATURE_SVE = 1u << 4,
  LIBPLATFORM_CPU_FEATURE_CRC32 = 1u << 5,
};

uint32_t platform_cpu_features(void);
const char *platform_simd_override(void);
