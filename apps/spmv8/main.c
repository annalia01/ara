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

#include <stdint.h>
#include <string.h>

#include "inc/spmv.h"
#include "../common/runtime.h"
#include "../common/util.h"


#include <stdio.h>

#define NR_LANES 8

extern uint64_t R;
extern uint64_t C;
extern uint64_t NZ;

extern uint8_t CSR_PROW[]
    __attribute__((aligned(4 * NR_LANES), section(".l2")));
extern uint8_t CSR_INDEX[]
    __attribute__((aligned(4 * NR_LANES), section(".l2")));
extern uint8_t CSR_DATA[] __attribute__((aligned(4 * NR_LANES), section(".l2")));
extern uint8_t CSR_IN_VECTOR[]
    __attribute__((aligned(4 * NR_LANES), section(".l2")));
extern uint8_t CSR_OUT_VECTOR[]
    __attribute__((aligned(4 * NR_LANES), section(".l2")));
static inline uint64_t read_mcycle(void) {
    uint64_t value;
    asm volatile ("csrr %0, cycle" : "=r"(value));
    return value;
}

static inline uint64_t read_minstret(void) {
    uint64_t value;
    asm volatile ("csrr %0, instret" : "=r"(value));
    return value;
}
int main() {
  printf("\n");
  printf("==========\n");
  printf("=  SpMV  =\n");
  printf("==========\n");
  printf("\n");
  printf("\n");

  double density = ((double)NZ) / (R * C);
  double nz_per_row = ((double)NZ) / R;

  printf("\n");
  printf(
      "-------------------------------------------------------------------\n");
  printf(
      "Calculating a (%d x %d) x %d sparse matrix vector multiplication...\n",
      R, C, C);
  printf("CSR format with %d nozeros: %f density, %f nonzeros per row \n", NZ,
         density, nz_per_row);
  printf(
      "-------------------------------------------------------------------\n");
  printf("\n");

  printf("calculating ... \n");
  uint64_t start_mcycle = read_mcycle();
uint64_t start_minstret = read_minstret();
  start_timer();
  spmv_csr_idx32_uint8(R, CSR_PROW, CSR_INDEX, CSR_DATA, CSR_IN_VECTOR,
                 CSR_OUT_VECTOR);
  stop_timer();
// Leggi i CSR dopo l’esecuzione
uint64_t end_mcycle = read_mcycle();
uint64_t end_minstret = read_minstret();

// Calcola differenze
uint64_t delta_mcycle = end_mcycle - start_mcycle;
uint64_t delta_minstret = end_minstret - start_minstret;

// Calcola IPC
double ipc = (double)delta_minstret / (double)delta_mcycle;

  // Metrics
  int64_t runtime = get_timer();
  float performance = 2.0 * NZ / runtime;
  float utilization = 100 * performance / (2.0 * NR_LANES);
printf("The execution took %lu cycles (CSR mcycle).\n", delta_mcycle);
printf("Instructions retired (CSR minstret): %lu\n", delta_minstret);
printf("IPC = %.3f\n", ipc);
  printf("The execution took %d cycles.\n", runtime);
  printf("The performance is %f FLOP/cycle (%f%% utilization) at %d lanes.\n",
         performance, utilization, NR_LANES);

  printf("Verifying ...\n");
  if (spmv_verify_uint8(R, CSR_PROW, CSR_INDEX, CSR_DATA, CSR_IN_VECTOR,
                  CSR_OUT_VECTOR)) {
    return 1;
  } else {
    printf("Passed.\n");
  }
  return 0;
}
