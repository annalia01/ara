#include <stdint.h>
#include "../../common/runtime.h"

int64_t M __attribute__((aligned(32))) = 3;
int64_t N __attribute__((aligned(32))) = 3;
int64_t F __attribute__((aligned(32))) = 3;

// Input: matrice 5x5 riempita con 1..25 (in row-major)
uint8_t i[(3+3-1)*(3+3-1)] __attribute__((aligned(32))) = {
     1,  2,  3,  4,  5,
     6,  7,  8,  9, 10,
    11, 12, 13, 14, 15,
    16, 17, 18, 19, 20,
    21, 22, 23, 24, 25
};

// Filtro 3x3: Sobel verticale semplificato
int8_t f[9] __attribute__((aligned(32))) = {
     1,  0, -1,
     1,  0, -1,
     1,  0, -1
};

// Output: 3x3, inizializzato a 0
int8_t o[9] __attribute__((aligned(32))) = {0};

// Golden output: atteso tutto -6
int8_t golden_o[9] __attribute__((aligned(32))) = {0};
void init_dataset() {
for (uint32_t r = 0; r < M; ++r) {
    for (uint32_t c = 0; c < N; ++c) {
      int8_t acc = 0;
      for (uint32_t fr = 0; fr < F; ++fr) {
        for (uint32_t fc = 0; fc < F; ++fc) {
          int8_t in_r = (int8_t)r + (int8_t)fr;
          int8_t in_c = (int8_t)c + (int8_t)fc;
          int8_t in_val = i[in_r * (N + F - 1) + in_c];
          int8_t f_val = f[fr * F + fc];
          acc += in_val * f_val;
        }
      }
      golden_o[r * N + c] = acc;
    }
  }
  }
