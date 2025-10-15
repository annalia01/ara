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

#include "../inc/iconv2d.h"
#include <stdio.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void iconv2d_5x5(int32_t *o, int32_t *i, int32_t *f, int64_t R, int64_t C,
                 int64_t F) {
  // We work on 2 rows of the output matrix at once
  int32_t block_size_o = 2;
  // We work on block_size_o + F - 1 rows of the input matrix at once

  // First iteration round, r = 0
  int32_t *i_ = i;
  int32_t *o_ = o;


  // Iterate over the output rows
  for (int32_t r = 0; r < R; r += block_size_o) {
    i_ = i + r * (C + F - 1);
    o_ = o + r * C;

    
    iconv2d_vec_4xC_5x5(o_, i, f, C, F, r);
    
  }
}


// Calculate 4 output matrix rows
void iconv2d_vec_4xC_5x5(int32_t *o, int32_t *i, int32_t *f, int64_t C,
                         int64_t F, int32_t r) {
int32_t debug_buf_v8[64] __attribute__((aligned(32)));
int32_t debug_buf_v0[64] __attribute__((aligned(32)));
  // Temporary variables (one filter column)
  int32_t t0, t1, t2, t3, t4;
  int32_t slamt;

  // Helper variables
  int32_t ldo = C << 2;
  int32_t ldi = (C + F - 1) << 2;
  int32_t ldf = F << 2;
  int32_t *f_;
  int32_t *i_=i+r*(C+F-1);

  // Compute on C elements
  asm volatile("vsetvli zero, %0, e32, m4, ta, ma" ::"r"(C + F - 1));
  asm volatile("vmv.v.i v0, 0");
  asm volatile("vmv.v.i v4, 0");
  // Fetch other 2 rows of the input matrix
  asm volatile("vle32.v v8, (%0); add %0, %0, %1" : "+&r"(i_) : "r"(ldi));
  
  asm volatile("vle32.v v12, (%0); add %0, %0, %1" : "+&r"(i_) : "r"(ldi));
  asm volatile("vle32.v v16, (%0); add %0, %0, %1" : "+&r"(i_) : "r"(ldi));
  asm volatile("vle32.v v20, (%0); add %0, %0, %1" : "+r"(i_) : "r"(ldi));
  asm volatile("vle32.v v24, (%0); add %0, %0, %1" : "+&r"(i_) : "r"(ldi));
  asm volatile("vle32.v v28, (%0); add %0, %0, %1" : "+&r"(i_) : "r"(ldi));

  // Compute on C elements
  asm volatile("vsetvli zero, %0, e32, m4, ta, ma" ::"r"(C));
  f_ = f;
  // Fetch the first column of the filter, and start calculating its
  // contribution on the two output rows (v0, v2)
  asm volatile("ld %1, (%0); add %0, %0, %2" : "+&r"(f_), "=&r"(t0) : "r"(ldf));
  asm volatile("vmacc.vx v0, %0, v8" ::"r"(t0));
  asm volatile("vmacc.vx v4, %0, v12" ::"r"(t0));

  asm volatile("ld %1, (%0); add %0, %0, %2" : "+&r"(f_), "=&r"(t1) : "r"(ldf));
  asm volatile("vmacc.vx v0, %0, v12" ::"r"(t1));
  asm volatile("vmacc.vx v4, %0, v16" ::"r"(t1));

  asm volatile("ld %1, (%0); add %0, %0, %2" : "+&r"(f_), "=&r"(t2) : "r"(ldf));
  asm volatile("vmacc.vx v0, %0, v16" ::"r"(t2));
  asm volatile("vmacc.vx v4, %0, v20" ::"r"(t2));

  asm volatile("ld %1, (%0); add %0, %0, %2" : "+&r"(f_), "=&r"(t3) : "r"(ldf));
  asm volatile("vmacc.vx v0, %0, v20" ::"r"(t3));
  asm volatile("vmacc.vx v4, %0, v24" ::"r"(t3));

  asm volatile("ld %1, (%0);" : "+&r"(f_), "=&r"(t4));
  asm volatile("vmacc.vx v0, %0, v24" ::"r"(t4));
  
  asm volatile("vmacc.vx v4, %0, v28" ::"r"(t4));

  for (int32_t idx = 1; idx < F - 1; ++idx) {
    // Adjust filter mtx pointer and slide-amount
    f_ = f + idx;
    slamt = idx;
    
    // Fetch the other columns of the filter (except for the last one), and
    // start calculating their contributions on the two output rows (v0, v2) To
    // do so, at each iteration slide down the input rows by one
     
    asm volatile("ld %1, (%0); add %0, %0, %2": "+&r"(f_), "=&r"(t0): "r"(ldf)); //iniziamo a caricare la seconda colonna del filtro 
    //asm volatile("vslidedown.vx v8, v8,  %0" ::"r"(slamt));
    int32_t *i__ = i + r*(C+F-1) + slamt;
    
    asm volatile("vle32.v v8, (%0); add %0, %0, %1" : "+&r"(i__) : "r"(ldi));
    
    asm volatile("vmacc.vx v0, %0, v8" ::"r"(t0));
   
    asm volatile("ld %1, (%0); add %0, %0, %2"
                 : "+&r"(f_), "=&r"(t1)
                 : "r"(ldf));
    //asm volatile("vslidedown.vx v18, v6,  %0" ::"r"(slamt));
    asm volatile("vle32.v v12, (%0); add %0, %0, %1" : "+&r"(i__) : "r"(ldi));
    asm volatile("vmacc.vx v0, %0, v12" ::"r"(t1));
    
    
    asm volatile("vmacc.vx v4, %0, v12" ::"r"(t0));

    asm volatile("ld %1, (%0); add %0, %0, %2"
                 : "+&r"(f_), "=&r"(t2)
                 : "r"(ldf));
    //asm volatile("vslidedown.vx v20, v8,  %0" ::"r"(slamt));
    asm volatile("vle32.v v16, (%0); add %0, %0, %1" : "+&r"(i__) : "r"(ldi));
    asm volatile("vmacc.vx v0, %0, v16" ::"r"(t2));
    
    
    asm volatile("vmacc.vx v4, %0, v16" ::"r"(t1));

    asm volatile("ld %1, (%0); add %0, %0, %2"
                 : "+&r"(f_), "=&r"(t3)
                 : "r"(ldf));
    //asm volatile("vslidedown.vx v22, v10, %0" ::"r"(slamt));
    asm volatile("vle32.v v20, (%0); add %0, %0, %1" : "+&r"(i__) : "r"(ldi));
    asm volatile("vmacc.vx v0, %0, v20" ::"r"(t3));
    
    asm volatile("vmacc.vx v4, %0, v20" ::"r"(t2));

    asm volatile("ld %1, (%0);" : "+&r"(f_), "=&r"(t4));
    //asm volatile("vslidedown.vx v24, v12, %0" ::"r"(slamt));
    asm volatile("vle32.v v24, (%0); add %0, %0, %1" : "+&r"(i__) : "r"(ldi));
    asm volatile("vmacc.vx v0, %0, v24" ::"r"(t4));
    
    asm volatile("vmacc.vx v4, %0, v24" ::"r"(t3));

    //asm volatile("vslidedown.vx v26, v14, %0" ::"r"(slamt));
    asm volatile("vle32.v v28, (%0); add %0, %0, %1" : "+&r"(i__) : "r"(ldi));
    asm volatile("vmacc.vx v4, %0, v28" ::"r"(t4));
  }

  f_ = f + (F - 1);
  slamt = (F - 1);
  // Repeat for the last filter column, and then store the output rows
  asm volatile("ld %1, (%0); add %0, %0, %2" : "+&r"(f_), "=&r"(t0) : "r"(ldf));
  //asm volatile("vslidedown.vx v16, v4,  %0" ::"r"(slamt));
  int32_t *i__= i + r*(C+F-1) + slamt;
  asm volatile("vle32.v v8, (%0); add %0, %0, %1" : "+&r"(i__) : "r"(ldi));
  
  asm volatile("vmacc.vx v0, %0, v8" ::"r"(t0));

  asm volatile("ld %1, (%0); add %0, %0, %2" : "+&r"(f_), "=&r"(t1) : "r"(ldf));
  //asm volatile("vslidedown.vx v18, v6,  %0" ::"r"(slamt));
  asm volatile("vle32.v v12, (%0); add %0, %0, %1" : "+&r"(i__) : "r"(ldi));
  asm volatile("vmacc.vx v0, %0, v12" ::"r"(t1));
  asm volatile("vmacc.vx v4, %0, v12" ::"r"(t0));

  asm volatile("ld %1, (%0); add %0, %0, %2" : "+&r"(f_), "=&r"(t2) : "r"(ldf));
  //asm volatile("vslidedown.vx v20, v8,  %0" ::"r"(slamt));
  asm volatile("vle32.v v16, (%0); add %0, %0, %1" : "+&r"(i__) : "r"(ldi));
  asm volatile("vmacc.vx v0, %0, v16" ::"r"(t2));
  asm volatile("vmacc.vx v4, %0, v16" ::"r"(t1));

  asm volatile("ld %1, (%0); add %0, %0, %2" : "+&r"(f_), "=&r"(t3) : "r"(ldf));
  //asm volatile("vslidedown.vx v22, v10, %0" ::"r"(slamt));
  asm volatile("vle32.v v20, (%0); add %0, %0, %1" : "+&r"(i__) : "r"(ldi));
  asm volatile("vmacc.vx v0, %0, v20" ::"r"(t3));
  asm volatile("vmacc.vx v4, %0, v20" ::"r"(t2));

  asm volatile("ld %1, (%0);" : "+&r"(f_), "=&r"(t4));
  //asm volatile("vslidedown.vx v24, v12, %0" ::"r"(slamt));
  asm volatile("vle32.v v24, (%0); add %0, %0, %1" : "+&r"(i__) : "r"(ldi));
  asm volatile("vmacc.vx v0, %0, v24" ::"r"(t4));
  asm volatile("vse32.v  v0, (%0); add %0, %0, %1" : "+&r"(o) : "r"(ldo));
  asm volatile("vmacc.vx v4, %0, v24" ::"r"(t3));

  //asm volatile("vslidedown.vx v26, v14, %0" ::"r"(slamt));
  asm volatile("vle32.v v28, (%0); add %0, %0, %1" : "+&r"(i__) : "r"(ldi));
  asm volatile("vmacc.vx v4, %0, v28" ::"r"(t4));
  asm volatile("vse32.v  v4, (%0); add %0, %0, %1" : "+&r"(o) : "r"(ldo));
}

