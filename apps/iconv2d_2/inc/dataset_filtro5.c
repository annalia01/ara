#include <stdint.h>
#include "../../../common/runtime.h"

int32_t M __attribute__((aligned(32))) = 5;
int32_t N __attribute__((aligned(32))) = 5;
int32_t F __attribute__((aligned(32))) = 5;

// Input: matrice 9x9 riempita con 1..81 (row-major)
int32_t i[(5+5-1)*(5+5-1)] __attribute__((aligned(32))) = {
     1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 25, 26, 27,
    28, 29, 30, 31, 32, 33, 34, 35, 36,
    37, 38, 39, 40, 41, 42, 43, 44, 45,
    46, 47, 48, 49, 50, 51, 52, 53, 54,
    55, 56, 57, 58, 59, 60, 61, 62, 63,
    64, 65, 66, 67, 68, 69, 70, 71, 72,
    73, 74, 75, 76, 77, 78, 79, 80, 81
};

// Filtro 5x5: pattern [+1, 0, -1, 0, +1] ripetuto
int32_t f[25] __attribute__((aligned(32))) = {
     1,  0, -1,  0,  1,
     1,  0, -1,  0,  1,
     1,  0, -1,  0,  1,
     1,  0, -1,  0,  1,
     1,  0, -1,  0,  1
};

// Output: 5x5, inizializzato a 0
int32_t o[25] __attribute__((aligned(32))) = {0};

// Golden output calcolato a mano (5x5)
int32_t golden_o[25] __attribute__((aligned(32))) = {0};

void init_dataset() {
for (uint32_t r = 0; r < M; ++r) {
    for (uint32_t c = 0; c < N; ++c) {
      int32_t acc = 0;
      for (uint32_t fr = 0; fr < F; ++fr) {
        for (uint32_t fc = 0; fc < F; ++fc) {
          int32_t in_r = r + fr;
          int32_t in_c = c + fc;
          int32_t in_val = i[in_r * (N + F - 1) + in_c];
          int32_t f_val = f[fr * F + fc];
          acc += in_val * f_val;
        }
      }
      golden_o[r * N + c] = acc;
    }
  }
  }
