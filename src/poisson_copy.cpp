#include "poisson.h"

// Create a PETSc vector for CPUs
void Poisson::create_vec(Vec *v, int size) {
  VecCreateSeq(PETSC_COMM_SELF, size * data->numCells, v);
}

// Destroy a PETSc vector
void Poisson::destroy_vec(Vec *v) {
  VecDestroy(v);
}

// Load a PETSc vector with values from an OP2 dat for CPUs
void Poisson::load_vec(Vec *v, op_dat v_dat, int size) {
  double *v_ptr;
  VecGetArray(*v, &v_ptr);
  op_arg vec_petsc_args[] = {
    op_arg_dat(v_dat, -1, OP_ID, size, "double", OP_READ)
  };
  op_mpi_halo_exchanges(data->cells, 1, vec_petsc_args);
  memcpy(v_ptr, (double *)v_dat->data, size * data->numCells * sizeof(double));
  op_mpi_set_dirtybit(1, vec_petsc_args);
  VecRestoreArray(*v, &v_ptr);
}

// Load an OP2 dat with the values from a PETSc vector for CPUs
void Poisson::store_vec(Vec *v, op_dat v_dat) {
  const double *v_ptr;
  VecGetArrayRead(*v, &v_ptr);
  op_arg vec_petsc_args[] = {
    op_arg_dat(v_dat, -1, OP_ID, 15, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges(data->cells, 1, vec_petsc_args);
  memcpy((double *)v_dat->data, v_ptr, 15 * data->numCells * sizeof(double));
  op_mpi_set_dirtybit(1, vec_petsc_args);
  VecRestoreArrayRead(*v, &v_ptr);
}

// Create a PETSc matrix for CPUs
void Poisson::create_mat(Mat *m, int row, int col, int prealloc) {
  MatCreate(PETSC_COMM_SELF, m);
  MatSetSizes(*m, PETSC_DECIDE, PETSC_DECIDE, row, col);
  MatSetType(*m, MATSEQAIJ);
  MatSeqAIJSetPreallocation(*m, prealloc, NULL);
}
