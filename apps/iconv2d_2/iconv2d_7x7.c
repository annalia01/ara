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

/*
  Optimized convolution for Ara
  The code is long only because of:
  1) Special cases related to the first/last 7 rows
  2) Unrolling of the loops to hide the latency of the moves, slides, mem ops

  At the end of the file, you can find the not-unrolled main loop in a comment,
  without the edge-code.

  Algorithm:
  a) Load the next input row
  b) Calculate its contributions to the F = 7 output rows using one column of
  the filter c) Slide down the input row by 1, injecting the next input scalar
  element in the tail d) Repeat from b), taking the next colum of the filter,
  until the last column is fetched e) Store the first output row, the one that
  is complete f) Move all the output rows up by one register, to restore the
  initial condition g) Repeat from a)

  Every time a new input row is loaded, a new output row is created.

  The first 6 rows and the last 6 rows do not follow this pattern, thus we wrote
  dedicated code. Because of the unrolling, we counted for this the first and
  last 7 rows, instead of 6

  This algorithm helps in minimizing the data dependencies, as every input rows
  is used To calculate 7 different output rows.
*/

#include "../inc/iconv2d.h"
#include <stdio.h>

void iconv2d_7x7(int32_t *o, int32_t *i, int32_t *f, int64_t M, int64_t N,
                 int64_t F) {

  unsigned long int block_size_n;

  // Set the vector configuration
  asm volatile("vsetvli %0, %1, e32, m2, ta, ma" : "=r"(block_size_n) : "r"(N));
  //puliamo gli accumulatori 
  asm volatile("vmv.v.i v16,  0");
  asm volatile("vmv.v.i v18,  0");
  asm volatile("vmv.v.i v20,  0");
  asm volatile("vmv.v.i v22,  0");
  asm volatile("vmv.v.i v24,  0");
  asm volatile("vmv.v.i v26,  0");
  asm volatile("vmv.v.i v28,  0");

  // Slice the matrix into a manageable number of columns n_
  for (unsigned long int n = 0; n < N; n += block_size_n) {
    // Set the vector length
    const unsigned long int n_ = MIN(N - n, block_size_n);
    // Find pointers to the submatrices
    int32_t *i_ = i + n;
    int32_t *o_ = o + n;

    asm volatile("vsetvli zero, %0, e32, m2, ta, ma" ::"r"(n_));

    iconv2d_7x7_block(o_, i_, f, M, N, n_, F);
  }
}

void iconv2d_7x7_block(int32_t *o, int32_t *i, int32_t *f, int64_t R, int64_t C,
                       int32_t n_, int64_t F) {

  // Helper variables
  int32_t ldo = C << 2;
  int32_t ldi_pad = (C + F - 1) << 2; //dimensione in byte della riga di input

  int32_t *i_ = i;

  int32_t t6, t13, t20, t27, t34, t41, t48, t0, t7, t14, t21, t0_1, t7_1, t14_1, t21_1, t28, t28_1, t35, t35_1, t42, t42_1;

  int32_t *i_slide_ptr_0;
  int32_t *i_slide_ptr_1;
  int32_t *i_slide_ptr_2;
  int32_t *i_slide_ptr_3;

  // Buffer some of the filter coefficients not to lose efficiency after a
  // vector store (CVA6 cannot issue memory operations if there is a pending
  // store!)
  //in questo caso vengono caricati ma vengono utilizzati dopo per evitare problemi di stallo e vengono caricati solo gli ultimi
//valori delle righe del filtro
  t6 = f[6];
  t13 = f[13];
  t20 = f[20];
  t27 = f[27];
  t34 = f[34];
  t41 = f[41];
  t48 = f[48];

  // Point to the scalar elements to insert during a slide
  i_slide_ptr_0 = i + n_ + 0 * (C + F - 1);
  i_slide_ptr_1 = i + n_ + 1 * (C + F - 1);
  i_slide_ptr_2 = i + n_ + 2 * (C + F - 1);
  i_slide_ptr_3 = i + n_ + 3 * (C + F - 1);

  ////////////////
  // Row 0 -> 3 //
  ////////////////
int64_t debug_buf[64] __attribute__((aligned(16)));
  // Load one input row
  asm volatile("vle32.v v0, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad));
  asm volatile("vle32.v v4, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad));
  asm volatile("vle32.v v8, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad));
  asm volatile("vle32.v v12, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad));

  // Main kernel, unrolled by 2
  for (int k = 0; k < F / 2; k++) {
  t0=f[0+(2*k)];
  t7=f[7+(2*k)];
  t14=f[14+(2*k)];
  t21=f[21+(2*k)];
      asm volatile("vmacc.vx v16, %0, v0" ::"r"(t0));
      asm volatile("vmacc.vx v16, %0, v4" ::"r"(t7));
    asm volatile("vmacc.vx v16, %0, v8" ::"r"(t14));
    asm volatile("vmacc.vx v16, %0, v12" ::"r"(t21));
    
      asm volatile("vmacc.vx v18, %0, v4" ::"r"(t0));
      asm volatile("vmacc.vx v18, %0, v8" ::"r"(t7));
      asm volatile("vmacc.vx v18, %0, v12" ::"r"(t14));
      
      asm volatile("vmacc.vx v20, %0, v8" ::"r"(t0));
      asm volatile("vmacc.vx v20, %0, v12" ::"r"(t7));
      
      asm volatile("vmacc.vx v22, %0, v12" ::"r"(t0));
      
    asm volatile("vslide1down.vx v2, v0, %0" ::"r"(*i_slide_ptr_0++));
    asm volatile("vslide1down.vx v6, v4, %0" ::"r"(*i_slide_ptr_1++));
    asm volatile("vslide1down.vx v10, v8, %0" ::"r"(*i_slide_ptr_2++));
    asm volatile("vslide1down.vx v14, v12, %0" ::"r"(*i_slide_ptr_3++));
    
t0_1=f[0+(2*k+1)];
  t7_1=f[7+(2*k+1)];
  t14_1=f[14+(2*k+1)];
  t21_1=f[21+(2*k+1)];
    asm volatile("vmacc.vx v16, %0, v2" ::"r"(t0_1));
    asm volatile("vmacc.vx v16, %0, v6" ::"r"(t7_1));
    asm volatile("vmacc.vx v16, %0, v10" ::"r"(t14_1));
    asm volatile("vmacc.vx v16, %0, v14" ::"r"(t21_1));
    
    asm volatile("vmacc.vx v18, %0, v6" ::"r"(t0_1));
    asm volatile("vmacc.vx v18, %0, v10" ::"r"(t7_1));
    asm volatile("vmacc.vx v18, %0, v14" ::"r"(t14_1));
    
    asm volatile("vmacc.vx v20, %0, v10" ::"r"(t0_1));
    asm volatile("vmacc.vx v20, %0, v14" ::"r"(t7_1));
    
    asm volatile("vmacc.vx v22, %0, v14" ::"r"(t0_1)); 
    
    asm volatile("vslide1down.vx v0, v2, %0" ::"r"(*i_slide_ptr_0++));
    asm volatile("vslide1down.vx v4, v6, %0" ::"r"(*i_slide_ptr_1++));
    asm volatile("vslide1down.vx v8, v10, %0" ::"r"(*i_slide_ptr_2++));
    asm volatile("vslide1down.vx v12, v14, %0" ::"r"(*i_slide_ptr_3++));
    
  }

  // Start calculating the next pointers to the elements to be slided in
  i_slide_ptr_0 = i + n_ + 0 * (C + F - 1);
  i_slide_ptr_1 = i + n_ + 1 * (C + F - 1);
  i_slide_ptr_2 = i + n_ + 2 * (C + F - 1);
//k=2, quindi manca l'ultimo slide moltiplicato per l'ultima colonna del filtro 
  // Main kernel, last iteration with filter coefficients reuse
  // Start loading next rows, from 4 to 6
  asm volatile("vmacc.vx v16, %0, v0" ::"r"(t6));
  asm volatile("vmacc.vx v16, %0, v4" ::"r"(t13));
  asm volatile("vmacc.vx v16, %0, v8" ::"r"(t20));
  asm volatile("vmacc.vx v16, %0, v12" ::"r"(t27));
  
  asm volatile("vmacc.vx v18, %0, v4" ::"r"(t6));
  asm volatile("vmacc.vx v18, %0, v8" ::"r"(t13));
  asm volatile("vmacc.vx v18, %0, v12" ::"r"(t20));
  
  asm volatile("vmacc.vx v20, %0, v8" ::"r"(t6));
  asm volatile("vmacc.vx v20, %0, v12" ::"r"(t13));
  
  asm volatile("vmacc.vx v22, %0, v12" ::"r"(t6));
  
  asm volatile("vle32.v v2, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad));  //carico in v2 la 5a riga
  asm volatile("vle32.v v6, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad)); //carico in v6 la 6a riga
  asm volatile("vle32.v v10, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad)); //carico in v10 la 7a riga
  

  ////////////////
  // Row 4 -> 6 //
  ////////////////

  // Main kernel, unrolled by 2
  for (int k = 0; k < F / 2; k++) {
  t0=f[0+(2*k)];
  t7=f[7+(2*k)];
  t14=f[14+(2*k)];
  t21=f[21+(2*k)];
  t28=f[28 + (2 * k)];
  t35=f[35 + (2 * k)];
  t42=f[42 + (2 * k)];
    asm volatile("vmacc.vx v16, %0, v2" ::"r"(t28));
    asm volatile("vmacc.vx v16, %0, v6" ::"r"(t35));
    asm volatile("vmacc.vx v16, %0, v10" ::"r"(t42));
    
    asm volatile("vmacc.vx v18, %0, v2" ::"r"(t21));
    asm volatile("vmacc.vx v18, %0, v6" ::"r"(t28));
    asm volatile("vmacc.vx v18, %0, v10" ::"r"(t35));
    
    
    asm volatile("vslide1down.vx v0, v2, %0" ::"r"(*i_slide_ptr_0++));

    
    asm volatile("vslide1down.vx v4, v6, %0" ::"r"(*i_slide_ptr_1++));

    asm volatile("vmacc.vx v20, %0, v2" ::"r"(t14));
    asm volatile("vmacc.vx v20, %0, v6" ::"r"(t21));
    asm volatile("vmacc.vx v20, %0, v10" ::"r"(t28));
    
    asm volatile("vslide1down.vx v8, v10, %0" ::"r"(*i_slide_ptr_2++));

    asm volatile("vmacc.vx v22, %0, v2" ::"r"(t7));
    asm volatile("vmacc.vx v22, %0, v6" ::"r"(t14));
    asm volatile("vmacc.vx v22, %0, v10" ::"r"(t21));

   
    asm volatile("vmacc.vx v24, %0, v2" ::"r"(t0));
    asm volatile("vmacc.vx v24, %0, v6" ::"r"(t7));
    asm volatile("vmacc.vx v24, %0, v10" ::"r"(t14));

    
    asm volatile("vmacc.vx v26, %0, v6" ::"r"(t0));
    asm volatile("vmacc.vx v26, %0, v10" ::"r"(t7));

  
    asm volatile("vmacc.vx v28, %0, v10" ::"r"(t0));
    
    t0_1=f[0+(2*k+1)];
  t7_1=f[7+(2*k+1)];
  t14_1=f[14+(2*k+1)];
  t21_1=f[21+(2*k+1)];
  
  t28_1=f[28 + (2 * k + 1)];
  t35_1=f[35 + (2 * k + 1)];
  t42_1=f[42 + (2 * k + 1)];
    asm volatile("vmacc.vx v16, %0, v0" ::"r"(t28_1));
    asm volatile("vmacc.vx v16, %0, v4" ::"r"(t35_1));
    asm volatile("vmacc.vx v16, %0, v8" ::"r"(t42_1));
    asm volatile("vslide1down.vx v2, v0, %0" ::"r"(*i_slide_ptr_0++));

    asm volatile("vmacc.vx v18, %0, v0" ::"r"(t21_1));
    asm volatile("vmacc.vx v18, %0, v4" ::"r"(t28_1));
    asm volatile("vmacc.vx v18, %0, v8" ::"r"(t35_1));
    asm volatile("vslide1down.vx v6, v4, %0" ::"r"(*i_slide_ptr_1++));

    asm volatile("vmacc.vx v20, %0, v0" ::"r"(t14_1));
    asm volatile("vmacc.vx v20, %0, v4" ::"r"(t21_1));
    asm volatile("vmacc.vx v20, %0, v8" ::"r"(t28_1));
    asm volatile("vslide1down.vx v10, v8, %0" ::"r"(*i_slide_ptr_2++));

    asm volatile("vmacc.vx v22, %0, v0" ::"r"(t7_1));
    asm volatile("vmacc.vx v22, %0, v4" ::"r"(t14_1));
    asm volatile("vmacc.vx v22, %0, v8" ::"r"(t21_1));

    asm volatile("vmacc.vx v24, %0, v0" ::"r"(t0_1));
    asm volatile("vmacc.vx v24, %0, v4" ::"r"(t7_1));
    asm volatile("vmacc.vx v24, %0, v8" ::"r"(t14_1));

    asm volatile("vmacc.vx v26, %0, v4" ::"r"(t0_1));
    asm volatile("vmacc.vx v26, %0, v8" ::"r"(t7_1));

    asm volatile("vmacc.vx v28, %0, v8" ::"r"(t0_1));
  }

  // Main kernel, last iteration with filter coefficients reuse
  asm volatile("vmacc.vx v16, %0, v2" ::"r"(t34));
  asm volatile("vmacc.vx v16, %0, v6" ::"r"(t41));
  asm volatile("vmacc.vx v16, %0, v10" ::"r"(t48));
  asm volatile("vse32.v v16, (%0); add %0, %0, %1" : "+&r"(o) : "r"(ldo));

  asm volatile("vmacc.vx v18, %0, v2" ::"r"(t27));
  asm volatile("vmacc.vx v18, %0, v6" ::"r"(t34));
  asm volatile("vmacc.vx v18, %0, v10" ::"r"(t41));
  asm volatile("vmv.v.v v16, v18");

  asm volatile("vmacc.vx v20, %0, v2" ::"r"(t20));
  asm volatile("vmacc.vx v20, %0, v6" ::"r"(t27));
  asm volatile("vmacc.vx v20, %0, v10" ::"r"(t34));
  asm volatile("vmv.v.v v18, v20");

  asm volatile("vmacc.vx v22, %0, v2" ::"r"(t13));
  asm volatile("vmacc.vx v22, %0, v6" ::"r"(t20));
  asm volatile("vmacc.vx v22, %0, v10" ::"r"(t27));
  asm volatile("vmv.v.v v20, v22");

  asm volatile("vmacc.vx v24, %0, v2" ::"r"(t6));
  asm volatile("vmacc.vx v24, %0, v6" ::"r"(t13));
  asm volatile("vmacc.vx v24, %0, v10" ::"r"(t20));
  asm volatile("vmv.v.v v22, v24");

  asm volatile("vmacc.vx v26, %0, v6" ::"r"(t6));
  asm volatile("vmacc.vx v26, %0, v10" ::"r"(t13));
  asm volatile("vmv.v.v v24, v26");

  asm volatile("vmacc.vx v28, %0, v10" ::"r"(t6));
  asm volatile("vmv.v.v v26, v28");

  ////////////
  // REGIME //
  ////////////

  // Start calculating the next pointers to the elements to be slided in
  i_slide_ptr_0 = i + n_;

  asm volatile("vle32.v v0, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad));

  // The following loop is unrolled by 2
  // The input matrix has R + F - 1 rows
  // We have computed F input rows already
  // Compute now until only F input rows are left
  // (The last F-1 rows do not contribute to F output rows each, so keep them
  // outside of this loop) (We keep F rows outside because of the unrolling by
  // 2, just for easeness)
  for (int j = 0; j < ((R + F - 1) - 2 * F) / 2; ++j) {
    // Work on F output rows

    //////////////
    // UNROLL 0 //
    //////////////

    // Main loop
    for (int k = 0; k < F / 2; ++k) {
      // Calculate F contributions of the input rows, on F different output rows
      t0=f[0+(2*k)];
  t7=f[7+(2*k)];
  t14=f[14+(2*k)];
  t21=f[21+(2*k)];
  t28=f[28 + (2 * k)];
  t35=f[35 + (2 * k)];
  t42=f[42 + (2 * k)];
      asm volatile("vmacc.vx v16, %0, v0" ::"r"(t42));
      asm volatile("vmacc.vx v18, %0, v0" ::"r"(t35));
      asm volatile("vmacc.vx v20, %0, v0" ::"r"(t28));
      asm volatile("vslide1down.vx v2, v0, %0" ::"r"(*i_slide_ptr_0++));
      asm volatile("vmacc.vx v22, %0, v0" ::"r"(t21));
      asm volatile("vmacc.vx v24, %0, v0" ::"r"(t14));
      asm volatile("vmacc.vx v26, %0, v0" ::"r"(t7));
      asm volatile("vmacc.vx v28, %0, v0" ::"r"(t0));
t0_1=f[0+(2*k+1)];
  t7_1=f[7+(2*k+1)];
  t14_1=f[14+(2*k+1)];
  t21_1=f[21+(2*k+1)];
t28_1=f[28 + (2 * k + 1)];
  t35_1=f[35 + (2 * k + 1)];
  t42_1=f[42 + (2 * k + 1)];
      // Calculate F contributions of the input rows, on F different output rows
      asm volatile("vmacc.vx v16, %0, v2" ::"r"(t42_1));
      asm volatile("vmacc.vx v18, %0, v2" ::"r"(t35_1));
      asm volatile("vmacc.vx v20, %0, v2" ::"r"(t28_1));
      asm volatile("vslide1down.vx v0, v2, %0" ::"r"(*i_slide_ptr_0++));
      asm volatile("vmacc.vx v22, %0, v2" ::"r"(t21_1));
      asm volatile("vmacc.vx v24, %0, v2" ::"r"(t14_1));
      asm volatile("vmacc.vx v26, %0, v2" ::"r"(t7_1));
      asm volatile("vmacc.vx v28, %0, v2" ::"r"(t0_1));
    }

    // Start calculating the next pointers to the elements to be slided in
    i_slide_ptr_1 = i + n_;

    // The last iteration is used to mask the latency of the store and the moves
    // Use buffered coefficients not to stall CVA6 for coherency
    asm volatile("vmacc.vx v16, %0, v0" ::"r"(t48));
    asm volatile("vse32.v  v16, (%0); add %0, %0, %1" : "+&r"(o) : "r"(ldo));
    asm volatile("vmacc.vx v18, %0, v0" ::"r"(t41));
    asm volatile("vmv.v.v v16, v18");
    asm volatile("vmacc.vx v20, %0, v0" ::"r"(t34));
    asm volatile("vle32.v v2, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad));
    asm volatile("vmv.v.v v18, v20");
    asm volatile("vmacc.vx v22, %0, v0" ::"r"(t27));
    asm volatile("vmacc.vx v24, %0, v0" ::"r"(t20));
    asm volatile("vmv.v.v v20, v22");
    asm volatile("vmacc.vx v26, %0, v0" ::"r"(t13));
    asm volatile("vmacc.vx v28, %0, v0" ::"r"(t6));
    asm volatile("vmv.v.v v22, v24");

    //////////////
    // UNROLL 1 //
    //////////////

    asm volatile("vmacc.vx v16, %0, v2" ::"r"(f[42]));
    asm volatile("vmacc.vx v18, %0, v2" ::"r"(f[35]));
    asm volatile("vmv.v.v v24, v26");
    asm volatile("vmacc.vx v20, %0, v2" ::"r"(f[28]));
    asm volatile("vslide1down.vx v0, v2, %0" ::"r"(*i_slide_ptr_1++));
    asm volatile("vmacc.vx v22, %0, v2" ::"r"(f[21]));
    asm volatile("vmv.v.v v26, v28");
    asm volatile("vmacc.vx v24, %0, v2" ::"r"(f[14]));
    asm volatile("vmacc.vx v26, %0, v2" ::"r"(f[7]));
    asm volatile("vmul.vx v28, v2, %0" ::"r"(f[0]));

    for (int k = 1; k < F; k += 2) {
      asm volatile("vmacc.vx v16, %0, v0" ::"r"(f[42 + k]));
      asm volatile("vmacc.vx v18, %0, v0" ::"r"(f[35 + k]));
      asm volatile("vmacc.vx v20, %0, v0" ::"r"(f[28 + k]));
      asm volatile("vslide1down.vx v2, v0, %0" ::"r"(*i_slide_ptr_1++));
      asm volatile("vmacc.vx v22, %0, v0" ::"r"(f[21 + k]));
      asm volatile("vmacc.vx v24, %0, v0" ::"r"(f[14 + k]));
      asm volatile("vmacc.vx v26, %0, v0" ::"r"(f[7 + k]));
      asm volatile("vmacc.vx v28, %0, v0" ::"r"(f[0 + k]));

      if (k == F - 2)
        break;

      asm volatile("vmacc.vx v16, %0, v2" ::"r"(f[42 + (k + 1)]));
      asm volatile("vmacc.vx v18, %0, v2" ::"r"(f[35 + (k + 1)]));
      asm volatile("vmacc.vx v20, %0, v2" ::"r"(f[28 + (k + 1)]));
      asm volatile("vslide1down.vx v0, v2, %0" ::"r"(*i_slide_ptr_1++));
      asm volatile("vmacc.vx v22, %0, v2" ::"r"(f[21 + (k + 1)]));
      asm volatile("vmacc.vx v24, %0, v2" ::"r"(f[14 + (k + 1)]));
      asm volatile("vmacc.vx v26, %0, v2" ::"r"(f[7 + (k + 1)]));
      asm volatile("vmacc.vx v28, %0, v2" ::"r"(f[0 + (k + 1)]));
    }

    // Start calculating the next pointers to the elements to be slided in
    i_slide_ptr_0 = i + n_;

    asm volatile("vmacc.vx v16, %0, v2" ::"r"(t48));
    asm volatile("vse32.v  v16, (%0); add %0, %0, %1" : "+&r"(o) : "r"(ldo));
    asm volatile("vmacc.vx v18, %0, v2" ::"r"(t41));
    asm volatile("vmv.v.v v16, v18");
    asm volatile("vmacc.vx v20, %0, v2" ::"r"(t34));
    asm volatile("vle32.v v0, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad));
    asm volatile("vmv.v.v v18, v20");
    asm volatile("vmacc.vx v22, %0, v2" ::"r"(t27));
    asm volatile("vmv.v.v v20, v22");
    asm volatile("vmacc.vx v24, %0, v2" ::"r"(t20));
    asm volatile("vmv.v.v v22, v24");
    asm volatile("vmacc.vx v26, %0, v2" ::"r"(t13));
    asm volatile("vmv.v.v v24, v26");
    asm volatile("vmacc.vx v28, %0, v2" ::"r"(t6));
    asm volatile("vmv.v.v v26, v28");
  }

  ////////////////////////
  // Row I-F -> (I-1)-3 //
  ////////////////////////

  // Point to the scalar elements to insert during a slide
  // i_slide_ptr_0 has already been computed
  i_slide_ptr_1 = i + n_ + 0 * (C + F - 1);
  i_slide_ptr_2 = i + n_ + 1 * (C + F - 1);
  i_slide_ptr_3 = i + n_ + 2 * (C + F - 1);

  // Load other three input rows (one was already loaded)
  asm volatile("vle32.v v4, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad));
  asm volatile("vle32.v v8, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad));
  asm volatile("vle32.v v12, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad));

  // Main kernel, unrolled by 2
  // Process 4 input rows
  for (int k = 0; k < F / 2; ++k) {
  t0=f[0+(2*k)];
  t7=f[7+(2*k)];
  t14=f[14+(2*k)];
  t21=f[21+(2*k)];
  t28=f[28 + (2 * k)];
  t35=f[35 + (2 * k)];
  t42=f[42 + (2 * k)];
    asm volatile("vslide1down.vx v2, v0, %0" ::"r"(*i_slide_ptr_0++));
    asm volatile("vmacc.vx v16, %0, v0" ::"r"(t42));
    asm volatile("vmacc.vx v18, %0, v0" ::"r"(t35));
    asm volatile("vmacc.vx v20, %0, v0" ::"r"(t28));
    asm volatile("vmacc.vx v22, %0, v0" ::"r"(t21));
    asm volatile("vmacc.vx v24, %0, v0" ::"r"(t14));
    asm volatile("vmacc.vx v26, %0, v0" ::"r"(t7));
    if (k == 0)
      asm volatile("vmul.vx v28, v0, %0" ::"r"(t0));
    else
      asm volatile("vmacc.vx v28, %0, v0" ::"r"(t0));
    asm volatile("vslide1down.vx v6, v4, %0" ::"r"(*i_slide_ptr_1++));
    asm volatile("vmacc.vx v18, %0, v4" ::"r"(t42));
    asm volatile("vmacc.vx v20, %0, v4" ::"r"(t35));
    asm volatile("vmacc.vx v22, %0, v4" ::"r"(t28));
    asm volatile("vmacc.vx v24, %0, v4" ::"r"(t21));
    asm volatile("vmacc.vx v26, %0, v4" ::"r"(t14));
    asm volatile("vmacc.vx v28, %0, v4" ::"r"(t7));
    asm volatile("vslide1down.vx v10, v8, %0" ::"r"(*i_slide_ptr_2++));
    asm volatile("vmacc.vx v20, %0, v8" ::"r"(t42));
    asm volatile("vmacc.vx v22, %0, v8" ::"r"(t35));
    asm volatile("vmacc.vx v24, %0, v8" ::"r"(t28));
    asm volatile("vmacc.vx v26, %0, v8" ::"r"(t21));
    asm volatile("vmacc.vx v28, %0, v8" ::"r"(t14));
    asm volatile("vslide1down.vx v14, v12, %0" ::"r"(*i_slide_ptr_3++));
    asm volatile("vmacc.vx v22, %0, v12" ::"r"(t42));
    asm volatile("vmacc.vx v24, %0, v12" ::"r"(t35));
    asm volatile("vmacc.vx v26, %0, v12" ::"r"(t28));
    asm volatile("vmacc.vx v28, %0, v12" ::"r"(t21));
    
    t0_1=f[0+(2*k+1)];
  t7_1=f[7+(2*k+1)];
  t14_1=f[14+(2*k+1)];
  t21_1=f[21+(2*k+1)];
  t28_1=f[28 + (2 * k + 1)];
  t35_1=f[35 + (2 * k + 1)];
  t42_1=f[42 + (2 * k + 1)];

    asm volatile("vslide1down.vx v0, v2, %0" ::"r"(*i_slide_ptr_0++));
    asm volatile("vmacc.vx v16, %0, v2" ::"r"(t42_1));
    asm volatile("vmacc.vx v18, %0, v2" ::"r"(t35_1));
    asm volatile("vmacc.vx v20, %0, v2" ::"r"(t28_1));
    asm volatile("vmacc.vx v22, %0, v2" ::"r"(t21_1));
    asm volatile("vmacc.vx v24, %0, v2" ::"r"(t14_1));
    asm volatile("vmacc.vx v26, %0, v2" ::"r"(t7_1));
    asm volatile("vmacc.vx v28, %0, v2" ::"r"(t0_1));
    asm volatile("vslide1down.vx v4, v6, %0" ::"r"(*i_slide_ptr_1++));
    asm volatile("vmacc.vx v18, %0, v6" ::"r"(t42_1));
    asm volatile("vmacc.vx v20, %0, v6" ::"r"(t35_1));
    asm volatile("vmacc.vx v22, %0, v6" ::"r"(t28_1));
    asm volatile("vmacc.vx v24, %0, v6" ::"r"(t21_1));
    asm volatile("vmacc.vx v26, %0, v6" ::"r"(t14_1));
    asm volatile("vmacc.vx v28, %0, v6" ::"r"(t7_1));
    asm volatile("vslide1down.vx v8, v10, %0" ::"r"(*i_slide_ptr_2++));
    asm volatile("vmacc.vx v20, %0, v10" ::"r"(t42_1));
    asm volatile("vmacc.vx v22, %0, v10" ::"r"(t35_1));
    asm volatile("vmacc.vx v24, %0, v10" ::"r"(t28_1));
    asm volatile("vmacc.vx v26, %0, v10" ::"r"(t21_1));
    asm volatile("vmacc.vx v28, %0, v10" ::"r"(t14_1));
    asm volatile("vslide1down.vx v12, v14, %0" ::"r"(*i_slide_ptr_3++));
    asm volatile("vmacc.vx v22, %0, v14" ::"r"(t42_1));
    asm volatile("vmacc.vx v24, %0, v14" ::"r"(t35_1));
    asm volatile("vmacc.vx v26, %0, v14" ::"r"(t28_1));
    asm volatile("vmacc.vx v28, %0, v14" ::"r"(t21_1));
  }

  // Start calculating the next pointers to the elements to be slided in
  i_slide_ptr_0 = i + n_ + 0 * (C + F - 1);
  i_slide_ptr_1 = i + n_ + 1 * (C + F - 1);
  i_slide_ptr_2 = i + n_ + 2 * (C + F - 1);

  asm volatile("vle32.v v2, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad));
  asm volatile("vmacc.vx v16, %0, v0" ::"r"(t48));
  asm volatile("vmacc.vx v18, %0, v0" ::"r"(t41));
  asm volatile("vmacc.vx v20, %0, v0" ::"r"(t34));
  asm volatile("vse32.v  v16, (%0); add %0, %0, %1" : "+&r"(o) : "r"(ldo));
  asm volatile("vmacc.vx v22, %0, v0" ::"r"(t27));
  asm volatile("vmacc.vx v24, %0, v0" ::"r"(t20));
  asm volatile("vmacc.vx v26, %0, v0" ::"r"(t13));
  asm volatile("vle32.v v6, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad));
  asm volatile("vmacc.vx v28, %0, v0" ::"r"(t6));
  asm volatile("vmacc.vx v18, %0, v4" ::"r"(t48));
  asm volatile("vmacc.vx v20, %0, v4" ::"r"(t41));
  asm volatile("vse32.v  v18, (%0); add %0, %0, %1" : "+&r"(o) : "r"(ldo));
  asm volatile("vmacc.vx v22, %0, v4" ::"r"(t34));
  asm volatile("vmacc.vx v24, %0, v4" ::"r"(t27));
  asm volatile("vmacc.vx v26, %0, v4" ::"r"(t20));
  asm volatile("vle32.v v10, (%0); add %0, %0, %1" : "+&r"(i) : "r"(ldi_pad));
  asm volatile("vmacc.vx v28, %0, v4" ::"r"(t13));
  asm volatile("vmacc.vx v20, %0, v8" ::"r"(t48));
  asm volatile("vmacc.vx v22, %0, v8" ::"r"(t41));
  asm volatile("vse32.v  v20, (%0); add %0, %0, %1" : "+&r"(o) : "r"(ldo));
  asm volatile("vmacc.vx v24, %0, v8" ::"r"(t34));
  asm volatile("vmacc.vx v26, %0, v8" ::"r"(t27));
  asm volatile("vmacc.vx v28, %0, v8" ::"r"(t20));
  asm volatile("vmacc.vx v22, %0, v12" ::"r"(t48));
  asm volatile("vse32.v  v22, (%0); add %0, %0, %1" : "+&r"(o) : "r"(ldo));
  asm volatile("vmacc.vx v24, %0, v12" ::"r"(t41));
  asm volatile("vmacc.vx v26, %0, v12" ::"r"(t34));
  asm volatile("vmacc.vx v28, %0, v12" ::"r"(t27));

  //////////////////////////
  // Row (I-1)-3 -> (I-1) //
  //////////////////////////

  // Main kernel, unrolled by 2
  for (int k = 0; k < F / 2; ++k) {
 
  t28=f[28 + (2 * k)];
  t35=f[35 + (2 * k)];
  t42=f[42 + (2 * k)];
    asm volatile("vslide1down.vx v0, v2, %0" ::"r"(*i_slide_ptr_0++));
    asm volatile("vmacc.vx v24, %0, v2" ::"r"(t42));
    asm volatile("vmacc.vx v26, %0, v2" ::"r"(t35));
    asm volatile("vslide1down.vx v4, v6, %0" ::"r"(*i_slide_ptr_1++));
    asm volatile("vmacc.vx v28, %0, v2" ::"r"(t28));
    asm volatile("vmacc.vx v26, %0, v6" ::"r"(t42));
    asm volatile("vslide1down.vx v8, v10, %0" ::"r"(*i_slide_ptr_2++));
    asm volatile("vmacc.vx v28, %0, v6" ::"r"(t35));
    asm volatile("vmacc.vx v28, %0, v10" ::"r"(t42));
t28_1=f[28 + (2 * k + 1)];
  t35_1=f[35 + (2 * k + 1)];
  t42_1=f[42 + (2 * k + 1)];
    asm volatile("vslide1down.vx v2, v0, %0" ::"r"(*i_slide_ptr_0++));
    asm volatile("vmacc.vx v24, %0, v0" ::"r"(t42_1));
    asm volatile("vmacc.vx v26, %0, v0" ::"r"(t35_1));
    asm volatile("vslide1down.vx v6, v4, %0" ::"r"(*i_slide_ptr_1++));
    asm volatile("vmacc.vx v28, %0, v0" ::"r"(t28_1));
    asm volatile("vmacc.vx v26, %0, v4" ::"r"(t42_1));
    asm volatile("vslide1down.vx v10, v8, %0" ::"r"(*i_slide_ptr_2++));
    asm volatile("vmacc.vx v28, %0, v4" ::"r"(t35_1));
    asm volatile("vmacc.vx v28, %0, v8" ::"r"(t42_1));
  }

  asm volatile("vmacc.vx v24, %0, v2" ::"r"(t48));
  asm volatile("vse32.v  v24, (%0); add %0, %0, %1" : "+&r"(o) : "r"(ldo));
  asm volatile("vmacc.vx v26, %0, v2" ::"r"(t41));
  asm volatile("vmacc.vx v28, %0, v2" ::"r"(t34));
  asm volatile("vmacc.vx v26, %0, v6" ::"r"(t48));
  asm volatile("vse32.v  v26, (%0); add %0, %0, %1" : "+&r"(o) : "r"(ldo));
  asm volatile("vmacc.vx v28, %0, v6" ::"r"(t41));
  asm volatile("vmacc.vx v28, %0, v10" ::"r"(t48));
  asm volatile("vse32.v  v28, (%0); add %0, %0, %1" : "+&r"(o) : "r"(ldo));
}
