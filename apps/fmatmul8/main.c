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

// Author: Matheus Cavalcante, ETH Zurich
//         Samuel Riedel, ETH Zurich
//         Matteo Perotti, ETH Zurich

#include <string.h>
#include <stdint.h>
#include "inc/fmatmul.h"
#include "../common/runtime.h"
#include "../common/util.h"

#include <stdio.h>
#define NR_LANES 8
// Define Matrix dimensions:
// C = AB with A=[MxN], B=[NxP], C=[MxP]
extern uint64_t M;
extern uint64_t N;
extern uint64_t P;

extern uint8_t a[] __attribute__((aligned(32 * NR_LANES), section(".l2")));
extern uint8_t b[] __attribute__((aligned(32 * NR_LANES), section(".l2")));
extern uint8_t c[] __attribute__((aligned(32 * NR_LANES), section(".l2")));
// Gold results
extern uint8_t g[] __attribute__((aligned(32 * NR_LANES), section(".l2")));

#define THRESHOLD 0.001

// Verify the matrix
int verify_matrix(uint8_t *result, uint8_t *gold, size_t R, size_t C,
                  uint8_t threshold) {
  for (uint64_t i = 0; i < R; ++i) {
    for (uint64_t j = 0; j < C; ++j) {
      uint64_t idx = i * C + j;
      if (!similarity_check(result[idx], gold[idx], threshold)) {
        return (i + j) == 0 ? -1 : idx;
      }
    }
  }
  return 0;
}

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
  printf("=============\n");
  printf("=  FMATMUL  =\n");
  printf("=============\n");
  printf("\n");
  printf("\n");


  for (uint64_t s = 4; s <= M; s *= 2) {

    printf("\n");
    printf("------------------------------------------------------------\n");
    printf("Calculating a (%d x %d) x (%d x %d) matrix multiplication...\n", s,
           s, s, s);
    printf("------------------------------------------------------------\n");
    printf("\n");

   // Leggi i CSR prima dell’esecuzione
uint64_t start_mcycle = read_mcycle();
uint64_t start_minstret = read_minstret();

// Esegui il kernel
start_timer();
fmatmul_uint8(c, a, b, s, s, s);
stop_timer();

// Leggi i CSR dopo l’esecuzione
uint64_t end_mcycle = read_mcycle();
uint64_t end_minstret = read_minstret();

// Calcola differenze
uint64_t delta_mcycle = end_mcycle - start_mcycle;
uint64_t delta_minstret = end_minstret - start_minstret;

// Calcola IPC
double ipc = (double)delta_minstret / (double)delta_mcycle;

// Metriche preesistenti
int64_t runtime = get_timer();
float performance = 2.0 * s * s * s / runtime;
float utilization = 100 * performance / (2.0 * NR_LANES);

// Stampa risultati
printf("The execution took %ld cycles (timer).\n", runtime);
printf("The execution took %lu cycles (CSR mcycle).\n", delta_mcycle);
printf("Instructions retired (CSR minstret): %lu\n", delta_minstret);
printf("IPC = %.3f\n", ipc);
printf("The performance is %f FLOP/cycle (%f%% utilization).\n",
       performance, utilization);

    // Verify the result only for s == M (to keep it simple)
    if (s == M) {
      printf("Verifying result...\n");
      int error = verify_matrix(c, g, s, s, THRESHOLD);
      if (error != 0) {
        printf("Error code %d\n", error);
        printf("c[%d]=%d\n", error, c[error]);
        return error;
      } else {
        printf("Passed.\n");
      }
    }
  }

  return 0;
}
