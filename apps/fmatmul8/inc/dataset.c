#include <stdint.h>
#include "../../common/runtime.h"
#define NR_LANES 8
// Definizione delle dimensioni delle matrici
uint64_t M = 32;
uint64_t N = 32;
uint64_t P = 32;

uint8_t a[32 * 32] __attribute__((aligned(32 * NR_LANES), section(".l2")));
uint8_t b[32 * 32] __attribute__((aligned(32 * NR_LANES), section(".l2")));
uint8_t c[32 * 32] __attribute__((aligned(32 * NR_LANES), section(".l2")));
uint8_t g[32 * 32] __attribute__((aligned(32 * NR_LANES), section(".l2")));

void init_dataset() {
  for (uint64_t i = 0; i < M; ++i) {
    for (uint64_t j = 0; j < N; ++j) {
      a[i * N + j] = (uint8_t)(i + j);
    }
  }

  for (uint64_t i = 0; i < N; ++i) {
    for (uint64_t j = 0; j < P; ++j) {
      b[i * P + j] = (uint8_t)(i == j);  // matrice identità
    }
  }

  // c inizialmente a zero
  for (uint64_t i = 0; i < M * P; ++i) {
    c[i] = 0.0;
    g[i] = 0.0;
  }

  // g = a × b (matrici piane)
  for (uint64_t i = 0; i < M; ++i) {
    for (uint64_t j = 0; j < P; ++j) {
      for (uint64_t k = 0; k < N; ++k) {
        g[i * P + j] += a[i * N + k] * b[k * P + j];
      }
    }
  }
}
