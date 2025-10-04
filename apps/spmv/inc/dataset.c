// Adatta dataset1.h al kernel SPMV
#include <stdint.h>
#include "dataset1.h"   // R, C, NNZ + val[], idx[], x[], ptr[], verify_data[]

/* Salva le costanti e poi elimina le macro per definire variabili con gli stessi nomi */
enum { R_CONST = R, C_CONST = C, NNZ_CONST = NNZ };
#undef R
#undef C
#undef NNZ

/* Variabili che il main/kernel si aspettano */
uint64_t R  = R_CONST;
uint64_t C  = C_CONST;
uint64_t NZ = NNZ_CONST;

/* Array attesi dal kernel SPMV */
int32_t CSR_PROW[R_CONST + 1];
int32_t CSR_INDEX[NNZ_CONST];    // <- in byte!
double  CSR_DATA [NNZ_CONST];
double  CSR_IN_VECTOR [C_CONST];
double  CSR_OUT_VECTOR[R_CONST]; // inizializzato a 0

/* (Opzionale) vettore golden se vuoi confrontare */
double  CSR_VERIFY[R_CONST];

/* Inizializza tutto prima di main() */
__attribute__((constructor))
static void spmv_dataset_init(void) {
  for (int i = 0; i < R_CONST + 1; ++i) CSR_PROW[i] = ptr[i];
  for (int i = 0; i < NNZ_CONST;   ++i) {
    CSR_INDEX[i] = idx[i] * 8;    // <<< conversione: colonna -> byte offset
    CSR_DATA[i]  = val[i];
  }
  for (int i = 0; i < C_CONST; ++i) CSR_IN_VECTOR[i]  = x[i];
  for (int i = 0; i < R_CONST; ++i) {
    CSR_OUT_VECTOR[i] = 0.0;
    CSR_VERIFY[i]     = verify_data[i]; // facoltativo
  }
}
