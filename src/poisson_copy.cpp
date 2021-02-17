#include "poisson.h"

void Poisson::copy_u(const double *u) {
  op_arg u_copy_args[] = {
    op_arg_dat(pU, -1, OP_ID, 15, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges(data->cells, 1, u_copy_args);
  memcpy(pU->data, u, data->numCells * 15 * sizeof(double));
  op_mpi_set_dirtybit(1, u_copy_args);
}

void Poisson::copy_rhs(double *rhs) {
  op_arg rhs_copy_args[] = {
    op_arg_dat(data->pRHS, -1, OP_ID, 15, "double", OP_READ)
  };
  op_mpi_halo_exchanges(data->cells, 1, rhs_copy_args);
  memcpy(rhs, pRHS->data, data->numCells * 15 * sizeof(double));
  op_mpi_set_dirtybit(1, rhs_copy_args);
}

void Poisson::create_vec(Vec *v) {
  VecCreateSeq(PETSC_COMM_SELF, 15 * data->numCells, v);
}

void Poisson::destroy_vec(Vec *v) {
  VecDestroy(v);
}

void Poisson::load_vec(Vec *v, op_dat v_dat) {
  double *v_ptr;
  VecGetArray(*v, &v_ptr);
  op_arg vec_petsc_args[] = {
    op_arg_dat(v_dat, -1, OP_ID, 15, "double", OP_READ)
  };
  op_mpi_halo_exchanges(data->cells, 1, vec_petsc_args);
  memcpy(v_ptr, (double *)v_dat->data, 15 * data->numCells * sizeof(double));
  op_mpi_set_dirtybit(1, vec_petsc_args);
  VecRestoreArray(*v, &v_ptr);
}

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

PetscErrorCode matAMult(Mat A, Vec x, Vec y) {
  Poisson *poisson;
  MatShellGetContext(A, poisson);
  const double *x_ptr;
  double *y_ptr;
  VecGetArrayRead(x, &x_ptr);
  VecGetArray(y, &y_ptr);

  poisson->rhs(x_ptr, y_ptr);

  VecRestoreArrayRead(x, &x_ptr);
  VecRestoreArray(y, &y_ptr);

  return 0;
}
