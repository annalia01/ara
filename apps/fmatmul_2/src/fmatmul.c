
// Copyright 2020 ETH Zurich and University of Bologna.
// SPDX-License-Identifier: Apache-2.0

#include "../inc/fmatmul.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define MIN(a,b) ((a)<(b)?(a):(b))
#define ASSUME_ALIGNED_64(p) ((float*)__builtin_assume_aligned((p), 64))



// --- Dispatcher --------------------------------------------------------------

void fmatmul(float * __restrict c_,
             const float * __restrict a_,
             const float * __restrict b_,
             const unsigned long int M,
             const unsigned long int N,
             const unsigned long int P) {
  float       * __restrict c = ASSUME_ALIGNED_64(c_);
  const float * __restrict a = ASSUME_ALIGNED_64(a_);
  const float * __restrict b = ASSUME_ALIGNED_64(b_);

    fmatmul_4x4(c, a, b, M, N, P);
  
}

// ============================================================================
// 4x4
// ============================================================================

void fmatmul_4x4(float *c, const float *a, const float *b,
                 const unsigned long int M, const unsigned long int N,
                 const unsigned long int P) {
  // We work on 4 rows of the matrix at once
  const unsigned long int block_size = 4;
  unsigned long int block_size_p;

  // Set the vector configuration
  asm volatile("vsetvli %0, %1, e32, m4, ta, ma" : "=r"(block_size_p) : "r"(P));

  // Slice the matrix into a manageable number of columns p_
  for (unsigned long int p = 0; p < P; p += block_size_p) {
    // Set the vector length
    const unsigned long int p_ = MIN(P - p, block_size_p);

    // Find pointers to the submatrices
    const float *b_ = b + p;
    float *c_ = c + p;

    asm volatile("vsetvli zero, %0, e32, m4, ta, ma" ::"r"(p_));

    // Iterate over the rows
    for (unsigned long int m = 0; m < M; m += block_size) {
      // Find pointer to the submatrices
      const float *a_ = a + m * N;
      float *c__ = c_ + m * P;

      fmatmul_vec_4x4_slice_init();
      fmatmul_vec_4x4(c__, a_, b_, N, P);
    }
  }
}

void fmatmul_vec_4x4_slice_init() {
  asm volatile("vmv.v.i v0,  0");
  asm volatile("vmv.v.i v4,  0");
  asm volatile("vmv.v.i v8,  0");
  asm volatile("vmv.v.i v12, 0");
}


void fmatmul_vec_4x4(float *c, const float *a, const float *b,
                       const unsigned long int N, const unsigned long int P) {
  unsigned long stride_a = N * sizeof(float);
  

  for (unsigned long k = 0; k < N; k++) {
    // Carica colonna k di A (4 elementi con stride N)
    asm volatile("vlse32.v v24, (%0), %1" :: "r"(a + k), "r"(stride_a));

    // Carica 4 scalari dalla riga k di B
    float b0 = b[k * P + 0];
    float b1 = b[k * P + 1];
    float b2 = b[k * P + 2];
    float b3 = b[k * P + 3];

    // Outer product: colonna(A) × scalari di B
    asm volatile("vfmacc.vf v0, %0, v24" :: "f"(b0));
    asm volatile("vfmacc.vf v4, %0, v24" :: "f"(b1));
    asm volatile("vfmacc.vf v8, %0, v24" :: "f"(b2));
    asm volatile("vfmacc.vf v12, %0, v24" :: "f"(b3));
  }

  // Scrivi il blocco 4×4 di C
  asm volatile("vse32.v v0, (%0)" :: "r"(c + 0*P));
  asm volatile("vse32.v v4, (%0)" :: "r"(c + 1*P));
  asm volatile("vse32.v v8, (%0)" :: "r"(c + 2*P));
  asm volatile("vse32.v v12, (%0)" :: "r"(c + 3*P));
}


