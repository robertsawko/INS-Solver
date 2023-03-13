#include "matrices/poisson_coarse_matrix.h"

#ifdef INS_MPI
#include "mpi_helper_func.h"
#endif

#include "dg_utils.h"
#include "dg_global_constants/dg_global_constants_2d.h"

int PoissonCoarseMatrix::getUnknowns() {
  const int setSize_ = _mesh->order->set->size;
  int unknowns = setSize_ * DG_NP_N1;
  return unknowns;
}

void PoissonCoarseMatrix::set_glb_ind() {
  int unknowns = getUnknowns();
  int global_ind = 0;
  #ifdef INS_MPI
  global_ind = get_global_mat_start_ind(unknowns);
  #endif
  op_arg args[] = {
    op_arg_dat(glb_ind, -1, OP_ID, 1, "int", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(_mesh->cells, 1, args);

  const int setSize = _mesh->cells->size;
  int *data_ptr = (int *)malloc(setSize * sizeof(int));
  cudaMemcpy(data_ptr, glb_ind->data_d, setSize * sizeof(int), cudaMemcpyDeviceToHost);

  #pragma omp parallel for
  for(int i = 0; i < _mesh->cells->size; i++) {
    data_ptr[i] = global_ind + i * DG_NP_N1;
  }

  cudaMemcpy(glb_ind->data_d, data_ptr, setSize * sizeof(int), cudaMemcpyHostToDevice);

  op_mpi_set_dirtybit_cuda(1, args);
  free(data_ptr);
}

void PoissonCoarseMatrix::setPETScMatrix() {
  if(!petscMatInit) {
    MatCreate(PETSC_COMM_WORLD, &pMat);
    petscMatInit = true;
    int unknowns = getUnknowns();
    MatSetSizes(pMat, unknowns, unknowns, PETSC_DECIDE, PETSC_DECIDE);

    #ifdef INS_MPI
    MatSetType(pMat, MATMPIAIJCUSPARSE);
    MatMPIAIJSetPreallocation(pMat, DG_NP_N1 * (DG_NUM_FACES + 1), NULL, 0, NULL);
    #else
    MatSetType(pMat, MATSEQAIJCUSPARSE);
    MatSeqAIJSetPreallocation(pMat, DG_NP_N1 * (DG_NUM_FACES + 1), NULL);
    #endif
    MatSetOption(pMat, MAT_NEW_NONZERO_ALLOCATION_ERR, PETSC_FALSE);
  }
  // Add cubature OP to Poisson matrix
  op_arg args[] = {
    op_arg_dat(op1, -1, OP_ID, DG_NP_N1 * DG_NP_N1, DG_FP_STR, OP_READ),
    op_arg_dat(glb_ind, -1, OP_ID, 1, "int", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(_mesh->cells, 2, args);

  const int setSize = _mesh->cells->size;
  DG_FP *op1_data = (DG_FP *)malloc(DG_NP_N1 * DG_NP_N1 * setSize * sizeof(DG_FP));
  int *glb   = (int *)malloc(setSize * sizeof(int));
  cudaMemcpy(op1_data, op1->data_d, setSize * DG_NP_N1 * DG_NP_N1 * sizeof(DG_FP), cudaMemcpyDeviceToHost);
  cudaMemcpy(glb, glb_ind->data_d, setSize * sizeof(int), cudaMemcpyDeviceToHost);
  op_mpi_set_dirtybit_cuda(2, args);

  #ifdef DG_COL_MAJ
  MatSetOption(pMat, MAT_ROW_ORIENTED, PETSC_FALSE);
  #else
  MatSetOption(pMat, MAT_ROW_ORIENTED, PETSC_TRUE);
  #endif

  for(int i = 0; i < setSize; i++) {
    int currentRow = glb[i];
    int currentCol = glb[i];
    int idxm[DG_NP_N1], idxn[DG_NP_N1];
    for(int n = 0; n < DG_NP_N1; n++) {
      idxm[n] = currentRow + n;
      idxn[n] = currentCol + n;
    }

    MatSetValues(pMat, DG_NP_N1, idxm, DG_NP_N1, idxn, &op1_data[i * DG_NP_N1 * DG_NP_N1], INSERT_VALUES);
  }

  free(op1_data);
  free(glb);

  op_arg edge_args[] = {
    op_arg_dat(op2[0], -1, OP_ID, DG_NP_N1 * DG_NP_N1, DG_FP_STR, OP_READ),
    op_arg_dat(op2[1], -1, OP_ID, DG_NP_N1 * DG_NP_N1, DG_FP_STR, OP_READ),
    op_arg_dat(glb_indL, -1, OP_ID, 1, "int", OP_READ),
    op_arg_dat(glb_indR, -1, OP_ID, 1, "int", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(_mesh->faces, 4, edge_args);
  DG_FP *op2L_data = (DG_FP *)malloc(DG_NP_N1 * DG_NP_N1 * _mesh->faces->size * sizeof(DG_FP));
  DG_FP *op2R_data = (DG_FP *)malloc(DG_NP_N1 * DG_NP_N1 * _mesh->faces->size * sizeof(DG_FP));
  int *glb_l = (int *)malloc(_mesh->faces->size * sizeof(int));
  int *glb_r = (int *)malloc(_mesh->faces->size * sizeof(int));

  cudaMemcpy(op2L_data, op2[0]->data_d, DG_NP_N1 * DG_NP_N1 * _mesh->faces->size * sizeof(DG_FP), cudaMemcpyDeviceToHost);
  cudaMemcpy(op2R_data, op2[1]->data_d, DG_NP_N1 * DG_NP_N1 * _mesh->faces->size * sizeof(DG_FP), cudaMemcpyDeviceToHost);
  cudaMemcpy(glb_l, glb_indL->data_d, _mesh->faces->size * sizeof(int), cudaMemcpyDeviceToHost);
  cudaMemcpy(glb_r, glb_indR->data_d, _mesh->faces->size * sizeof(int), cudaMemcpyDeviceToHost);

  // Add Gauss OP and OPf to Poisson matrix
  for(int i = 0; i < _mesh->faces->size; i++) {
    int leftRow = glb_l[i];
    int rightRow = glb_r[i];

    int idxl[DG_NP_N1], idxr[DG_NP_N1];
    for(int n = 0; n < DG_NP_N1; n++) {
      idxl[n] = leftRow + n;
      idxr[n] = rightRow + n;
    }

    MatSetValues(pMat, DG_NP_N1, idxl, DG_NP_N1, idxr, &op2L_data[i * DG_NP_N1 * DG_NP_N1], INSERT_VALUES);
    MatSetValues(pMat, DG_NP_N1, idxr, DG_NP_N1, idxl, &op2R_data[i * DG_NP_N1 * DG_NP_N1], INSERT_VALUES);
  }

  free(op2L_data);
  free(op2R_data);
  free(glb_l);
  free(glb_r);

  op_mpi_set_dirtybit_cuda(4, edge_args);

  MatAssemblyBegin(pMat, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(pMat, MAT_FINAL_ASSEMBLY);
}