#include "poisson.h"

void Poisson::create_vec(Vec *v, int size) {
  VecCreateSeqCUDA(PETSC_COMM_SELF, size * data->numCells, v);
}

void Poisson::destroy_vec(Vec *v) {
  VecDestroy(v);
}

void Poisson::load_vec(Vec *v, op_dat v_dat, int size) {
  double *v_ptr;
  VecCUDAGetArray(*v, &v_ptr);
  op_arg vec_petsc_args[] = {
    op_arg_dat(v_dat, -1, OP_ID, size, "double", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(data->cells, 1, vec_petsc_args);
  cudaMemcpy(v_ptr, (double *)v_dat->data_d, size * data->numCells * sizeof(double), cudaMemcpyDeviceToDevice);
  op_mpi_set_dirtybit_cuda(1, vec_petsc_args);
  VecCUDARestoreArray(*v, &v_ptr);
}

void Poisson::store_vec(Vec *v, op_dat v_dat) {
  const double *v_ptr;
  VecCUDAGetArrayRead(*v, &v_ptr);
  op_arg vec_petsc_args[] = {
    op_arg_dat(v_dat, -1, OP_ID, 15, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(data->cells, 1, vec_petsc_args);
  cudaMemcpy((double *)v_dat->data_d, v_ptr, 15 * data->numCells * sizeof(double), cudaMemcpyDeviceToDevice);
  op_mpi_set_dirtybit_cuda(1, vec_petsc_args);
  VecCUDARestoreArrayRead(*v, &v_ptr);
}

void Poisson::create_mat(Mat *m, int row, int col, int prealloc) {
  MatCreate(PETSC_COMM_SELF, m);
  MatSetSizes(*m, PETSC_DECIDE, PETSC_DECIDE, row, col);
  MatSetType(*m, MATSEQAIJCUSPARSE);
  MatSeqAIJSetPreallocation(*m, prealloc, NULL);
}
