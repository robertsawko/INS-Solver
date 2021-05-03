#include "cblas.h"

#include "op_seq.h"
#include "../blas_calls.h"

inline void openblas_cubature_mm(const int numCells, const double *temp,
                                 double *mm) {
  for(int i = 0; i < numCells; i++) {
    const double *temp_c = temp + i * 46 * 15;
    double *mm_c = mm + i * 15 * 15;

    cblas_dgemm(CblasColMajor, CblasNoTrans, CblasTrans, 15, 15, 46, 1.0, constants->cubV, 15, temp_c, 15, 0.0, mm_c, 15);
  }
}

void cubature_mm_blas(INSData *nsData, CubatureData *cubData) {
  // Make sure OP2 data is in the right place
  op_arg mm_cubature_args[] = {
    op_arg_dat(cubData->temp, -1, OP_ID, 46 * 15, "double", OP_READ),
    op_arg_dat(cubData->mm, -1, OP_ID, 15 * 15, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges(nsData->cells, 2, mm_cubature_args);

  openblas_cubature_mm(nsData->numCells, (double *)cubData->temp->data,
                       (double *)cubData->mm->data);

  // Set correct dirty bits for OP2
  op_mpi_set_dirtybit(2, mm_cubature_args);
}
