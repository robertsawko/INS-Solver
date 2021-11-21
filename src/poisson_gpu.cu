#include "poisson.h"

#ifdef INS_MPI
#include "mpi_helper_func.h"
#endif

// Copy PETSc vec array to OP2 dat
void PoissonSolve::copy_vec_to_dat(op_dat dat, const double *dat_d) {
  op_arg copy_args[] = {
    op_arg_dat(dat, -1, OP_ID, DG_NP, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(dat->set, 1, copy_args);
  cudaMemcpy(dat->data_d, dat_d, dat->set->size * DG_NP * sizeof(double), cudaMemcpyDeviceToDevice);
  op_mpi_set_dirtybit_cuda(1, copy_args);
}

// Copy OP2 dat to PETSc vec array
void PoissonSolve::copy_dat_to_vec(op_dat dat, double *dat_d) {
  op_arg copy_args[] = {
    op_arg_dat(dat, -1, OP_ID, DG_NP, "double", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(dat->set, 1, copy_args);
  cudaMemcpy(dat_d, dat->data_d, dat->set->size * DG_NP * sizeof(double), cudaMemcpyDeviceToDevice);
  op_mpi_set_dirtybit_cuda(1, copy_args);
}

// Create a PETSc vector for GPUs
void PoissonSolve::create_vec(Vec *v, int size) {
  VecCreate(PETSC_COMM_WORLD, v);
  VecSetType(*v, VECCUDA);
  VecSetSizes(*v, size * mesh->cells->size, PETSC_DECIDE);
}

// Destroy a PETSc vector
void PoissonSolve::destroy_vec(Vec *v) {
  VecDestroy(v);
}

// Load a PETSc vector with values from an OP2 dat for GPUs
void PoissonSolve::load_vec(Vec *v, op_dat v_dat, int size) {
  double *v_ptr;
  VecCUDAGetArray(*v, &v_ptr);
  op_arg vec_petsc_args[] = {
    op_arg_dat(v_dat, -1, OP_ID, size, "double", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(mesh->cells, 1, vec_petsc_args);
  cudaMemcpy(v_ptr, (double *)v_dat->data_d, size * v_dat->set->size * sizeof(double), cudaMemcpyDeviceToDevice);
  op_mpi_set_dirtybit_cuda(1, vec_petsc_args);
  VecCUDARestoreArray(*v, &v_ptr);
}

// Load an OP2 dat with the values from a PETSc vector for GPUs
void PoissonSolve::store_vec(Vec *v, op_dat v_dat) {
  const double *v_ptr;
  VecCUDAGetArrayRead(*v, &v_ptr);
  op_arg vec_petsc_args[] = {
    op_arg_dat(v_dat, -1, OP_ID, DG_NP, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(mesh->cells, 1, vec_petsc_args);
  cudaMemcpy((double *)v_dat->data_d, v_ptr, DG_NP * v_dat->set->size * sizeof(double), cudaMemcpyDeviceToDevice);
  op_mpi_set_dirtybit_cuda(1, vec_petsc_args);
  VecCUDARestoreArrayRead(*v, &v_ptr);
}

PetscErrorCode matAMult(Mat A, Vec x, Vec y) {
  PoissonSolve *poisson;
  MatShellGetContext(A, &poisson);
  const double *x_ptr;
  double *y_ptr;
  VecCUDAGetArrayRead(x, &x_ptr);
  VecCUDAGetArray(y, &y_ptr);

  poisson->calc_rhs(x_ptr, y_ptr);

  VecCUDARestoreArrayRead(x, &x_ptr);
  VecCUDARestoreArray(y, &y_ptr);
  return 0;
}

void PoissonSolve::create_shell_mat(Mat *m) {
  MatCreateShell(PETSC_COMM_WORLD, DG_NP * mesh->cells->size, DG_NP * mesh->cells->size, PETSC_DETERMINE, PETSC_DETERMINE, this, m);
  MatShellSetOperation(*m, MATOP_MULT, (void(*)(void))matAMult);
  MatShellSetVecType(*m, VECCUDA);
}

PetscErrorCode precon(PC pc, Vec x, Vec y) {
  PoissonSolve *poisson;
  PCShellGetContext(pc, (void **)&poisson);
  const double *x_ptr;
  double *y_ptr;
  VecCUDAGetArrayRead(x, &x_ptr);
  VecCUDAGetArray(y, &y_ptr);

  poisson->precond(x_ptr, y_ptr);

  VecCUDARestoreArrayRead(x, &x_ptr);
  VecCUDARestoreArray(y, &y_ptr);
  return 0;
}

void PoissonSolve::set_shell_pc(PC pc) {
  PCShellSetApply(pc, precon);
  PCShellSetContext(pc, this);
}

void PoissonSolve::setGlbInd() {
  int global_ind = 0;
  #ifdef INS_MPI
  global_ind = get_global_start_index(glb_ind->set);
  #endif
  op_arg args[] = {
    op_arg_dat(glb_ind, -1, OP_ID, 1, "int", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(mesh->cells, 1, args);
  int *data_ptr = (int *)malloc(mesh->cells->size * sizeof(int));
  cudaMemcpy(data_ptr, glb_ind->data_d, glb_ind->set->size * sizeof(int), cudaMemcpyDeviceToHost);
  for(int i = 0; i < mesh->cells->size; i++) {
    data_ptr[i] = global_ind + i;
  }
  cudaMemcpy(glb_ind->data_d, data_ptr, glb_ind->set->size * sizeof(int), cudaMemcpyHostToDevice);
  op_mpi_set_dirtybit_cuda(1, args);
  free(data_ptr);
}

void PoissonSolve::setMatrix() {
  if(pMatInit) {
    MatDestroy(&pMat);
  }
  MatCreate(PETSC_COMM_WORLD, &pMat);
  pMatInit = true;
  MatSetSizes(pMat, DG_NP * mesh->cells->size, DG_NP * mesh->cells->size, PETSC_DECIDE, PETSC_DECIDE);

  #ifdef INS_MPI
  MatSetType(pMat, MATMPIAIJCUSPARSE);
  MatMPIAIJSetPreallocation(pMat, DG_NP * 4, NULL, 0, NULL);
  #else
  MatSetType(pMat, MATSEQAIJCUSPARSE);
  MatSeqAIJSetPreallocation(pMat, DG_NP * 4, NULL);
  #endif
  MatSetOption(pMat, MAT_NEW_NONZERO_ALLOCATION_ERR, PETSC_FALSE);

  // Add cubature OP to Poisson matrix
  op_arg args[] = {
    op_arg_dat(op1, -1, OP_ID, DG_NP * DG_NP, "double", OP_READ),
    op_arg_dat(glb_ind, -1, OP_ID, 1, "int", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(mesh->cells, 2, args);
  double *op1_data = (double *)malloc(DG_NP * DG_NP * mesh->cells->size * sizeof(double));
  int *glb = (int *)malloc(mesh->cells->size * sizeof(int));
  cudaMemcpy(op1_data, op1->data_d, op1->set->size * DG_NP * DG_NP * sizeof(double), cudaMemcpyDeviceToHost);
  cudaMemcpy(glb, glb_ind->data_d, glb_ind->set->size * sizeof(int), cudaMemcpyDeviceToHost);
  op_mpi_set_dirtybit_cuda(2, args);

  for(int i = 0; i < mesh->cells->size; i++) {
    int global_ind = glb[i];
    // Convert data to row major format
    for(int m = 0; m < DG_NP; m++) {
      for(int n = 0; n < DG_NP; n++) {
        int row = global_ind * DG_NP + m;
        int col = global_ind * DG_NP + n;
        double val = op1_data[i * DG_NP * DG_NP + m * DG_NP + n];
        MatSetValues(pMat, 1, &row, 1, &col, &val, INSERT_VALUES);
      }
    }
  }

  free(op1_data);
  free(glb);

  op_arg edge_args[] = {
    op_arg_dat(op2[0], -1, OP_ID, DG_NP * DG_NP, "double", OP_READ),
    op_arg_dat(op2[1], -1, OP_ID, DG_NP * DG_NP, "double", OP_READ),
    op_arg_dat(glb_indL, -1, OP_ID, 1, "int", OP_READ),
    op_arg_dat(glb_indR, -1, OP_ID, 1, "int", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(mesh->edges, 4, edge_args);
  double *op2L_data = (double *)malloc(DG_NP * DG_NP * mesh->edges->size * sizeof(double));
  double *op2R_data = (double *)malloc(DG_NP * DG_NP * mesh->edges->size * sizeof(double));
  int *glb_l = (int *)malloc(mesh->edges->size * sizeof(int));
  int *glb_r = (int *)malloc(mesh->edges->size * sizeof(int));

  cudaMemcpy(op2L_data, op2[0]->data_d, DG_NP * DG_NP * mesh->edges->size * sizeof(double), cudaMemcpyDeviceToHost);
  cudaMemcpy(op2R_data, op2[1]->data_d, DG_NP * DG_NP * mesh->edges->size * sizeof(double), cudaMemcpyDeviceToHost);
  cudaMemcpy(glb_l, glb_indL->data_d, mesh->edges->size * sizeof(int), cudaMemcpyDeviceToHost);
  cudaMemcpy(glb_r, glb_indR->data_d, mesh->edges->size * sizeof(int), cudaMemcpyDeviceToHost);

  // Add Gauss OP and OPf to Poisson matrix
  for(int i = 0; i < mesh->edges->size; i++) {
    int leftElement = glb_l[i];
    int rightElement = glb_r[i];

    // Gauss OPf
    // Convert data to row major format
    for(int m = 0; m < DG_NP; m++) {
      for(int n = 0; n < DG_NP; n++) {
        int row = leftElement * DG_NP + m;
        int col = rightElement * DG_NP + n;
        double val = op2L_data[i * DG_NP * DG_NP + m * DG_NP + n];
        MatSetValues(pMat, 1, &row, 1, &col, &val, INSERT_VALUES);
      }
    }
    // Convert data to row major format
    for(int m = 0; m < DG_NP; m++) {
      for(int n = 0; n < DG_NP; n++) {
        int row = rightElement * DG_NP + m;
        int col = leftElement * DG_NP + n;
        double val = op2R_data[i * DG_NP * DG_NP + m * DG_NP + n];
        MatSetValues(pMat, 1, &row, 1, &col, &val, INSERT_VALUES);
      }
    }
  }

  free(op2L_data);
  free(op2R_data);
  free(glb_l);
  free(glb_r);

  op_mpi_set_dirtybit_cuda(4, edge_args);

  MatAssemblyBegin(pMat, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(pMat, MAT_FINAL_ASSEMBLY);

}