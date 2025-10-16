#include <stdint.h>
#include "../../../common/runtime.h"

int32_t M __attribute__((aligned(32))) = 5;
int32_t N __attribute__((aligned(32))) = 5;
int32_t F __attribute__((aligned(32))) = 7;

// Input: matrice 11x11 riempita con 1..121 (row-major)
uint8_t i[(5+7-1)*(5+7-1)] __attribute__((aligned(32))) = {
     1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11,
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
    23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
    34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
    67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77,
    78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
    89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
   100,101,102,103,104,105,106,107,108,109,110,
   111,112,113,114,115,116,117,118,119,120,121
};

// Filtro 7x7: pattern [+1, 0, -1, 0, +1, 0, -1] ripetuto
int8_t f[49] __attribute__((aligned(32))) = {
     1,  0, -1,  0,  1,  0, -1,
     1,  0, -1,  0,  1,  0, -1,
     1,  0, -1,  0,  1,  0, -1,
     1,  0, -1,  0,  1,  0, -1,
     1,  0, -1,  0,  1,  0, -1,
     1,  0, -1,  0,  1,  0, -1,
     1,  0, -1,  0,  1,  0, -1
};

// Output: 5x5, inizializzato a 0
int8_t o[25] __attribute__((aligned(32))) = {0};

// Golden output calcolato a mano (5x5)
int8_t golden_o[25] __attribute__((aligned(32))) = {0};

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
