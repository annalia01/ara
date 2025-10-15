// Copyright 2022 ETH Zurich and University of Bologna.
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

// Author: Chi Zhang, ETH Zurich <chizhang@iis.ee.ethz.ch>

#include "../inc/spmv.h"
#include "../../../common/runtime.h"
#include "../../../common/util.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>


#define SLICE_SIZE 128
#define DATA_BYTE 8 // double type has 8 bytes

void spmv_csr_idx32_uint8(int32_t N_ROW, uint8_t *CSR_PROW, uint8_t *CSR_INDEX,
                    uint8_t *CSR_DATA, uint8_t *IN_VEC, uint8_t *OUT_VEC) {
                    int8_t debug_buf[7] __attribute__((aligned(16)));
  // Pre-carica la riga 0
  uint8_t len   = CSR_PROW[1] - CSR_PROW[0];
  uint8_t *data  = CSR_DATA  + CSR_PROW[0];
  uint8_t *index= CSR_INDEX + CSR_PROW[0];

  for (int i = 0; i < N_ROW; ++i) {
    // --- reset accumulatore ---
    asm volatile("vsetvli zero, %0, e8, m4, ta, ma" :: "r"(1));
    asm volatile("vmv.v.i v16, 0");

    // --- loop vettoriale dinamico ---
    while (len > 0) {
      size_t vl;
      asm volatile("vsetvli %0, %1, e8, m4, ta, ma"
                   : "=r"(vl) : "r"(len));

      asm volatile("vle8.v v4, (%0)" :: "r"(data));   // carica valori
     
      asm volatile("vle8.v v8, (%0)" :: "r"(index));  // carica indici
         
      asm volatile("vloxei8.v v0, (%0), v8" :: "r"(IN_VEC)); // gather da x
      asm volatile("vmul.vv v12, v4, v0");            // moltiplicazione
      asm volatile("vredsum.vs v16, v12, v16");       // riduzione

      data  += vl;
      index += vl;
      len   -= vl;
    }

    // --- store risultato riga corrente ---
    uint8_t tmp;
    asm volatile("vmv.x.s %0, v16" : "=r"(tmp));
    OUT_VEC[i] = (uint8_t)tmp;

    // --- pre-carica riga successiva ---
    if (i + 1 < N_ROW) {
      len   = CSR_PROW[i + 2] - CSR_PROW[i + 1];
      data  = CSR_DATA  + CSR_PROW[i + 1];
      index = CSR_INDEX + CSR_PROW[i + 1];
    }
  }
}

int spmv_verify_uint8(int32_t N_ROW, uint8_t *CSR_PROW, uint8_t *CSR_INDEX,
                uint8_t *CSR_DATA, uint8_t *IN_VEC, uint8_t *OUT_VEC) {
  for (int32_t i = 0; i < N_ROW; ++i) {
    uint8_t res = OUT_VEC[i];

    uint8_t len = CSR_PROW[i + 1] - CSR_PROW[i];
    uint8_t *data = CSR_DATA + CSR_PROW[i];
    uint8_t *index = CSR_INDEX + CSR_PROW[i];

    uint8_t golden = 0;
    for (int32_t j = 0; j < len; ++j) {
      uint8_t idx = index[j];
      golden = golden + data[j] * IN_VEC[idx];
      // printf("index:%d, data: %f, vec: %f\n", idx, data[j], IN_VEC[idx]);
    }
    if ((uint8_t)golden != (uint8_t)res) {
      printf("Sorry, wrong value! at index %d, result = %f, golden = %f \n", i,
             res, golden);
      return i;
    }
  }
  return 0;
}
