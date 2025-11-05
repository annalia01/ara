#include <stdint.h>
#include "dataset1.h"   // R, C, NNZ + val[], idx[], x[], ptr[], verify_data[]

enum { R_CONST = R, C_CONST = C, NNZ_CONST = NNZ };
#undef R
#undef C
#undef NNZ


uint64_t R  = R_CONST;
uint64_t C  = C_CONST;
uint64_t NZ = NNZ_CONST;


int32_t CSR_PROW[R_CONST + 1];
int32_t CSR_INDEX[NNZ_CONST];    // <- in byte!
float  CSR_DATA [NNZ_CONST];
float  CSR_IN_VECTOR [C_CONST];
float  CSR_OUT_VECTOR[R_CONST]; // inizializzato a 0


float  CSR_VERIFY[R_CONST];


__attribute__((constructor))
static void spmv_dataset_init(void) {
  for (int i = 0; i < R_CONST + 1; ++i) CSR_PROW[i] = ptr[i];
  for (int i = 0; i < NNZ_CONST;   ++i) {
    CSR_INDEX[i] = idx[i] * sizeof(float);    // <<< conversione: colonna -> byte offset
    CSR_DATA[i]  = val[i];
  }
  for (int i = 0; i < C_CONST; ++i) CSR_IN_VECTOR[i]  = x[i];
  for (int i = 0; i < R_CONST; ++i) {
    CSR_OUT_VECTOR[i] = 0.0;
    CSR_VERIFY[i]     = verify_data[i]; // facoltativo
  }
}
