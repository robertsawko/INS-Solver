#include "poisson.h"

#ifdef INS_MPI
#include "mpi_helper_func.h"
#endif

// Copy u PETSc vec array to OP2 dat (TODO avoid this copy)
void Poisson_MF2::copy_u(const double *u_d) {
  op_arg u_copy_args[] = {
    op_arg_dat(u, -1, OP_ID, 15, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(mesh->cells, 1, u_copy_args);
  cudaMemcpy(u->data_d, u_d, u->set->size * 15 * sizeof(double), cudaMemcpyDeviceToDevice);
  op_mpi_set_dirtybit_cuda(1, u_copy_args);
}

// Copy rhs OP2 dat to PETSc vec array (TODO avoid this copy)
void Poisson_MF2::copy_rhs(double *rhs_d) {
  op_arg rhs_copy_args[] = {
    op_arg_dat(rhs, -1, OP_ID, 15, "double", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(mesh->cells, 1, rhs_copy_args);
  cudaMemcpy(rhs_d, rhs->data_d, rhs->set->size * 15 * sizeof(double), cudaMemcpyDeviceToDevice);
  op_mpi_set_dirtybit_cuda(1, rhs_copy_args);
}

// Create a PETSc vector for GPUs
void Poisson::create_vec(Vec *v, int size) {
  VecCreate(PETSC_COMM_WORLD, v);
  VecSetType(*v, VECCUDA);
  VecSetSizes(*v, size * mesh->cells->size, PETSC_DECIDE);
}

// Destroy a PETSc vector
void Poisson::destroy_vec(Vec *v) {
  VecDestroy(v);
}

// Load a PETSc vector with values from an OP2 dat for GPUs
void Poisson::load_vec(Vec *v, op_dat v_dat, int size) {
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
void Poisson::store_vec(Vec *v, op_dat v_dat) {
  const double *v_ptr;
  VecCUDAGetArrayRead(*v, &v_ptr);
  op_arg vec_petsc_args[] = {
    op_arg_dat(v_dat, -1, OP_ID, 15, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(mesh->cells, 1, vec_petsc_args);
  cudaMemcpy((double *)v_dat->data_d, v_ptr, 15 * v_dat->set->size * sizeof(double), cudaMemcpyDeviceToDevice);
  op_mpi_set_dirtybit_cuda(1, vec_petsc_args);
  VecCUDARestoreArrayRead(*v, &v_ptr);
}

// Create a PETSc matrix for GPUs
void Poisson::create_mat(Mat *m, int row, int col, int prealloc0, int prealloc1) {
  MatCreate(PETSC_COMM_WORLD, m);
  MatSetSizes(*m, row, col, PETSC_DECIDE, PETSC_DECIDE);

  #ifdef INS_MPI
  MatSetType(*m, MATMPIAIJCUSPARSE);
  MatMPIAIJSetPreallocation(*m, prealloc0, NULL, prealloc1, NULL);
  #else
  MatSetType(*m, MATSEQAIJCUSPARSE);
  MatSeqAIJSetPreallocation(*m, prealloc0, NULL);
  #endif
  MatSetOption(*m, MAT_NEW_NONZERO_ALLOCATION_ERR, PETSC_FALSE);
}

PetscErrorCode matAMult2(Mat A, Vec x, Vec y) {
  timer->startLinearSolveMFMatMult();
  Poisson_MF2 *poisson;
  MatShellGetContext(A, &poisson);
  const double *x_ptr;
  double *y_ptr;
  VecCUDAGetArrayRead(x, &x_ptr);
  VecCUDAGetArray(y, &y_ptr);

  poisson->calc_rhs(x_ptr, y_ptr);

  VecCUDARestoreArrayRead(x, &x_ptr);
  VecCUDARestoreArray(y, &y_ptr);
  timer->endLinearSolveMFMatMult();
  return 0;
}

void Poisson_MF2::create_shell_mat(Mat *m) {
  MatCreateShell(PETSC_COMM_WORLD, 15 * mesh->cells->size, 15 * mesh->cells->size, PETSC_DETERMINE, PETSC_DETERMINE, this, m);
  MatShellSetOperation(*m, MATOP_MULT, (void(*)(void))matAMult2);
  MatShellSetVecType(*m, VECCUDA);
}

void Poisson_M::setGlbInd() {
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

void Poisson_M::createMassMatrix() {
  create_mat(&pMMat, 15 * mesh->cells->size, 15 * mesh->cells->size, 15);
  pMMatInit = true;
  // Add Cubature OP to mass matrix
  double *cub_MM = (double *)malloc(15 * 15 * mesh->cells->size * sizeof(double));
  int *glb       = (int *)malloc(mesh->cells->size * sizeof(int));
  op_arg args[] = {
    op_arg_dat(mesh->cubature->mm, -1, OP_ID, 15 * 15, "double", OP_READ),
    op_arg_dat(glb_ind, -1, OP_ID, 1, "int", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(mesh->cells, 2, args);
  cudaMemcpy(cub_MM, mesh->cubature->mm->data_d, mesh->cubature->mm->set->size * 15 * 15 * sizeof(double), cudaMemcpyDeviceToHost);
  cudaMemcpy(glb, glb_ind->data_d, glb_ind->set->size * sizeof(int), cudaMemcpyDeviceToHost);
  op_mpi_set_dirtybit_cuda(2, args);

  for(int i = 0; i < mesh->cells->size; i++) {
    // Convert data to row major format
    int global_ind = glb[i];
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int row = global_ind * 15 + m;
        int col = global_ind * 15 + n;
        int colInd = n * 15 + m;
        double val = cub_MM[i * 15 * 15 + colInd];
        MatSetValues(pMMat, 1, &row, 1, &col, &val, INSERT_VALUES);
      }
    }
  }

  free(cub_MM);
  free(glb);

  MatAssemblyBegin(pMMat, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(pMMat, MAT_FINAL_ASSEMBLY);
}

void Poisson_M::createMatrix() {
  create_mat(&pMat, 15 * mesh->cells->size, 15 * mesh->cells->size, 15 * 4);
  pMatInit = true;
  double tol = 1e-15;

  // Add cubature OP to Poisson matrix
  op_arg args[] = {
    op_arg_dat(op1, -1, OP_ID, 15 * 15, "double", OP_READ),
    op_arg_dat(glb_ind, -1, OP_ID, 1, "int", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(mesh->cells, 2, args);
  double *op1_data = (double *)malloc(15 * 15 * mesh->cells->size * sizeof(double));
  int *glb = (int *)malloc(mesh->cells->size * sizeof(int));
  cudaMemcpy(op1_data, op1->data_d, op1->set->size * 15 * 15 * sizeof(double), cudaMemcpyDeviceToHost);
  cudaMemcpy(glb, glb_ind->data_d, glb_ind->set->size * sizeof(int), cudaMemcpyDeviceToHost);
  op_mpi_set_dirtybit_cuda(2, args);

  for(int i = 0; i < mesh->cells->size; i++) {
    int global_ind = glb[i];
    // Convert data to row major format
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int row = global_ind * 15 + m;
        int col = global_ind * 15 + n;
        double val = op1_data[i * 15 * 15 + m * 15 + n];
        MatSetValues(pMat, 1, &row, 1, &col, &val, INSERT_VALUES);
      }
    }
  }

  free(op1_data);
  free(glb);

  op_arg edge_args[] = {
    op_arg_dat(op2[0], -1, OP_ID, 15 * 15, "double", OP_READ),
    op_arg_dat(op2[1], -1, OP_ID, 15 * 15, "double", OP_READ),
    op_arg_dat(glb_indL, -1, OP_ID, 1, "int", OP_READ),
    op_arg_dat(glb_indR, -1, OP_ID, 1, "int", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(mesh->edges, 4, edge_args);
  double *op2L_data = (double *)malloc(15 * 15 * mesh->edges->size * sizeof(double));
  double *op2R_data = (double *)malloc(15 * 15 * mesh->edges->size * sizeof(double));
  int *glb_l = (int *)malloc(mesh->edges->size * sizeof(int));
  int *glb_r = (int *)malloc(mesh->edges->size * sizeof(int));

  cudaMemcpy(op2L_data, op2[0]->data_d, 15 * 15 * mesh->edges->size * sizeof(double), cudaMemcpyDeviceToHost);
  cudaMemcpy(op2R_data, op2[1]->data_d, 15 * 15 * mesh->edges->size * sizeof(double), cudaMemcpyDeviceToHost);
  cudaMemcpy(glb_l, glb_indL->data_d, mesh->edges->size * sizeof(int), cudaMemcpyDeviceToHost);
  cudaMemcpy(glb_r, glb_indR->data_d, mesh->edges->size * sizeof(int), cudaMemcpyDeviceToHost);

  // Add Gauss OP and OPf to Poisson matrix
  for(int i = 0; i < mesh->edges->size; i++) {
    int leftElement = glb_l[i];
    int rightElement = glb_r[i];

    // Gauss OPf
    // Convert data to row major format
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int row = leftElement * 15 + m;
        int col = rightElement * 15 + n;
        double val = op2L_data[i * 15 * 15 + m * 15 + n];
        MatSetValues(pMat, 1, &row, 1, &col, &val, INSERT_VALUES);
      }
    }
    // Convert data to row major format
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int row = rightElement * 15 + m;
        int col = leftElement * 15 + n;
        double val = op2R_data[i * 15 * 15 + m * 15 + n];
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

void Poisson_M::createBCMatrix() {
  create_mat(&pBCMat, 15 * mesh->cells->size, 21 * mesh->cells->size, 15);
  pBCMatInit = true;
  double tol = 1e-15;

  op_arg args[] = {
    op_arg_dat(op_bc, -1, OP_ID, 7 * 15, "double", OP_READ),
    op_arg_dat(glb_indBC, -1, OP_ID, 1, "int", OP_READ),
    op_arg_dat(mesh->bedgeNum, -1, OP_ID, 1, "int", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(mesh->bedges, 3, args);

  double *op_data = (double *)malloc(7 * 15 * mesh->bedges->size * sizeof(double));
  int *glb        = (int *)malloc(mesh->bedges->size * sizeof(int));
  int *edgeNum    = (int *)malloc(mesh->bedges->size * sizeof(int));

  cudaMemcpy(op_data, op_bc->data_d, 7 * 15 * mesh->bedges->size * sizeof(double), cudaMemcpyDeviceToHost);
  cudaMemcpy(glb, glb_indBC->data_d, mesh->bedges->size * sizeof(int), cudaMemcpyDeviceToHost);
  cudaMemcpy(edgeNum, mesh->bedgeNum->data_d, mesh->bedges->size * sizeof(int), cudaMemcpyDeviceToHost);

  // Create BCs matrix using Gauss data on boundary edges
  for(int i = 0; i < mesh->bedges->size; i++) {
    int global_ind = glb[i];
    for(int j = 0; j < 7 * 15; j++) {
      int col = global_ind * 21 + edgeNum[i] * 7 + (j % 7);
      int row = global_ind * 15 + (j / 7);
      double val = op_data[i * 7 * 15 + j];
      MatSetValues(pBCMat, 1, &row, 1, &col, &val, INSERT_VALUES);
    }
  }

  free(op_data);
  free(glb);
  free(edgeNum);

  op_mpi_set_dirtybit_cuda(3, args);

  MatAssemblyBegin(pBCMat, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(pBCMat, MAT_FINAL_ASSEMBLY);
}
