// Copyright 2020 ETH Zurich and University of Bologna.
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: Matteo Perotti

#include <stdint.h>
#include <string.h>

#include "inc/iconv2d.h"
#include "../common/runtime.h"

#include "../common/util.h"

void init_dataset();
#ifdef SPIKEGEM
#define NR_LANES 8
#include <stdio.h>
#else
#include "../common/printf.h"
#endif
// Define Matrix dimensions:
// o = i ° f, with i=[MxN], f=[FxF], o=[MxN]
// The filter is a square matrix, and F is odd

// Matrices defined in data.S
extern int32_t i[] __attribute__((
    aligned(4 * NR_LANES))); // [ (M+floor(F/2)) * (N+floor(F/2)) ]
extern int32_t f[] __attribute__((aligned(4 * NR_LANES)));        // [ F*F ]
extern int32_t o[] __attribute__((aligned(4 * NR_LANES)));        // [ M*N ]
extern int32_t golden_o[] __attribute__((aligned(4 * NR_LANES))); // [ M*N ]
extern int32_t M;
extern int32_t N;
extern int32_t F;

// Verify the matrices
int verify_matrix(int32_t *matrix, int32_t *golden_matrix, int64_t R,
                  int64_t C) {
  for (int r = 0; r < R; ++r)
    for (int c = 0; c < C; ++c)
      if (matrix[c + C * r] != golden_matrix[c + C * r]) {
        printf("Error: o[%d][%d] = %ld, instead of %ld\n", r, c,
               matrix[c + C * r], golden_matrix[c + C * r]);
        return 1;
      }
  return 0;
}

void print_matrix(int32_t const *matrix, uint64_t num_rows,
                  uint64_t num_columns) {
  printf("0x%8X\n", matrix);
  for (int32_t i = 0; i < num_rows; ++i) {
    for (int32_t j = 0; j < num_columns; ++j) {
      printf("%10d ", matrix[i * num_columns + j]);
    }
    printf("\n");
  }
}
static inline uint64_t read_minstret(void) {
   uint64_t value;
   asm volatile ("csrr %0, instret" : "=r"(value));
   return value;
}
int main() {
  printf("\n");
  printf("=============\n");
  printf("=  ICONV2D  =\n");
  printf("=============\n");
  printf("\n");
  printf("\n");

  // Call the main kernel, and measure cycles
  init_dataset();
  #ifdef SPIKEGEM
  uint64_t start_minstret = read_minstret();
  #endif
  start_timer();
    iconv2d_3x3(o, i, f, M, N, F);
  stop_timer();
  #ifdef SPIKEGEM
  uint64_t end_minstret = read_minstret();
  uint64_t delta_minstret = end_minstret - start_minstret;
  #endif
  // Performance metrics
  int64_t runtime = get_timer();
  float performance = 2.0 * F * F * M * N / runtime;
  float utilization = 100 * performance / (2.0 * NR_LANES);

  printf("The execution took %d cycles.\n", runtime);
  #ifdef SPIKEGEM
  printf("Instructions retired (CSR minstret): %lu\n", delta_minstret);
  #endif
  printf("The performance is %f OP/cycle (%f%% utilization).\n", performance,
         utilization);
print_matrix(o, M, N);
  // Verify correctness
  printf("Verifying result...\n");
  int error = verify_matrix(o, golden_o, M, N);
  if (error != 0) {
    printf("Fail.\n");
  } else {
    printf("Passed.\n");
  }

  return error;
}
