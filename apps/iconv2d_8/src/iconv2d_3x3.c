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

#include "../../common/printf.h"
#define MIN(a, b) ((a) < (b) ? (a) : (b))



void iconv2d_3x3_uint8(uint8_t *o, uint8_t *i, uint8_t *f, int64_t R, int64_t C,
                 int64_t F) {
printf("entro in iconv2d\n");
  // We work on 4 rows of the output matrix at once
  uint8_t block_size_o = 3;
  // We work on block_size_o + F - 1 rows of the input matrix at once

  // First iteration round, r = 0

  uint8_t *o_ = o;


  // Temporary variables
  int t0, t1, t2, t3, t4, t5, t6, t7, t8;
  uint8_t ldi = (C + F - 1);
  uint8_t ldf = F ;
  uint8_t *f_;
  f_ = f;
  asm volatile("lbu %1, (%0); add %0, %0, %2" : "+&r"(f_), "=&r"(t0) : "r"(ldf));
  asm volatile("lbu %1, (%0); add %0, %0, %2" : "+&r"(f_), "=&r"(t1) : "r"(ldf));
  asm volatile("lbu %1, (%0);" : "+&r"(f_), "=&r"(t2));
printf("fatte le prime load\n");
  f_ = f + 1;
  // Fetch the middle column of the filter, and start calculating its
  // contributions on the output rows To do so, slide down the input rows by one
  asm volatile("lbu %1, (%0); add %0, %0, %2" : "+&r"(f_), "=&r"(t3) : "r"(ldf));
  asm volatile("lbu %1, (%0); add %0, %0, %2" : "+&r"(f_), "=&r"(t4) : "r"(ldf));
  asm volatile("lbu %1, (%0);" : "+&r"(f_), "=&r"(t5));
  f_ = f + 2;
  // Repeat for the last filter column, and then store the output rows
  asm volatile("lbu %1, (%0); add %0, %0, %2" : "+&r"(f_), "=&r"(t6) : "r"(ldf));
  asm volatile("lbu %1, (%0); add %0, %0, %2" : "+&r"(f_), "=&r"(t7) : "r"(ldf));
  asm volatile("lbu %1, (%0);" : "+&r"(f_), "=&r"(t8));

for (int32_t r = 0; r < R; r += block_size_o) {


printf("entro nel ciclo for\n");
    // Fetch C + F - 1 elements (padding included)
  uint8_t ldo = C ;
  uint8_t *i___=i+r*(C+F-1);
  uint8_t *i_col = i + r*(C+F-1) + 1;

  // Fetch C + F - 1 elements (padding included)
  asm volatile("vsetvli zero, %0, e8, m4, ta, ma" ::"r"(C + F - 1));
  asm volatile("vmv.v.i v0, 0");
  asm volatile("vmv.v.i v4, 0");
  asm volatile("vmv.v.i v8, 0");

  asm volatile("vle8.v v12,  (%0); add %0, %0, %1" : "+&r"(i___) : "r"(ldi));

  asm volatile("vle8.v v16, (%0); add %0, %0, %1" : "+&r"(i___) : "r"(ldi));


  asm volatile("vle8.v v20, (%0); add %0, %0, %1" : "+&r"(i___) : "r"(ldi));


  asm volatile("vmul.vx v0, v12, %0" ::"r"(t0));

  asm volatile("vmul.vx v4, v16, %0" ::"r"(t0));
  asm volatile("vle8.v v24, (%0); add %0, %0, %1" : "+&r"(i___) : "r"(ldi));

  asm volatile("vmacc.vx v0, %0, v16" ::"r"(t1));

  asm volatile("vmacc.vx v4, %0, v20" ::"r"(t1));
  asm volatile("vle8.v v28, (%0); add %0, %0, %1" : "+&r"(i___) : "r"(ldi));

  asm volatile("vmacc.vx v0, %0, v20" ::"r"(t2));

  asm volatile("vle8.v v12,  (%0); add %0, %0, %1" : "+&r"(i_col) : "r"(ldi));
  asm volatile("vmul.vx v8, v20, %0" ::"r"(t0));


  asm volatile("vsetvli zero, %0, e8, m4, ta, ma" ::"r"(C));

  asm volatile("vmacc.vx v8, %0, v24" ::"r"(t1));

  asm volatile("vle8.v v16,  (%0); add %0, %0, %1" : "+&r"(i_col) : "r"(ldi));
  asm volatile("vmacc.vx v4, %0, v24" ::"r"(t2));

  asm volatile("vmacc.vx v8, %0, v28" ::"r"(t2));

  asm volatile("vle8.v v20,  (%0); add %0, %0, %1" : "+&r"(i_col) : "r"(ldi));

  asm volatile("vmacc.vx v0, %0, v12" ::"r"(t3));

  asm volatile("vmacc.vx v0, %0, v16" ::"r"(t4));
  asm volatile("vle8.v v24,  (%0); add %0, %0, %1" : "+&r"(i_col) : "r"(ldi));

  asm volatile("vmacc.vx v4, %0, v16" ::"r"(t3));

  asm volatile("vmacc.vx v0, %0, v20" ::"r"(t5));
  asm volatile("vmacc.vx v4, %0, v20" ::"r"(t4));

  asm volatile("vle8.v v28,  (%0); add %0, %0, %1" : "+&r"(i_col) : "r"(ldi));
  asm volatile("vmacc.vx v8, %0, v20" ::"r"(t3));

  asm volatile("vmacc.vx v4, %0, v24" ::"r"(t5));
  asm volatile("vmacc.vx v8, %0, v24" ::"r"(t4));


  asm volatile("vmacc.vx v8, %0, v28" ::"r"(t5));

  i_col = i + r*(C+F-1) + 2;


  asm volatile("vle8.v v12,  (%0); add %0, %0, %1" : "+&r"(i_col) : "r"(ldi));

  asm volatile("vle8.v v16,  (%0); add %0, %0, %1" : "+&r"(i_col) : "r"(ldi));

  asm volatile("vmacc.vx v0, %0, v12" ::"r"(t6));

  asm volatile("vmacc.vx v0, %0, v16" ::"r"(t7));

  asm volatile("vle8.v v20,  (%0); add %0, %0, %1" : "+&r"(i_col) : "r"(ldi));


  asm volatile("vmacc.vx v4, %0, v16" ::"r"(t6));

  // Compute on C elements

  asm volatile("vmacc.vx v0, %0, v20" ::"r"(t8));

  asm volatile("vse8.v  v0, (%0); add %0, %0, %1" : "+&r"(o_) : "r"(ldo));

  asm volatile("vle8.v v24,  (%0); add %0, %0, %1" : "+&r"(i_col) : "r"(ldi));

  asm volatile("vmacc.vx v4, %0, v20" ::"r"(t7));
  asm volatile("vmacc.vx v8, %0, v20" ::"r"(t6));

  asm volatile("vmacc.vx v4, %0, v24" ::"r"(t8));
  if(r+1<R) asm volatile("vse8.v  v4, (%0); add %0, %0, %1" : "+&r"(o_) : "r"(ldo));

  asm volatile("vle8.v v28,  (%0); add %0, %0, %1" : "+&r"(i_col) : "r"(ldi));

  asm volatile("vmacc.vx v8, %0, v24" ::"r"(t7));

  asm volatile("vmacc.vx v8, %0, v28" ::"r"(t8));

  if(r+2<R) asm volatile("vse8.v  v8, (%0); add %0, %0, %1" : "+&r"(o_) : "r"(ldo));

  }
}
