#include "linear_solvers/petsc_utils.h"

#include "dg_utils.h"

using namespace PETScUtils;

// Copy PETSc vec array to OP2 dat
void PETScUtils::copy_vec_to_dat(op_dat dat, const double *dat_d) {
  op_arg copy_args[] = {
    op_arg_dat(dat, -1, OP_ID, DG_NP, "double", OP_WRITE)
  };
  op_mpi_halo_exchanges_cuda(dat->set, 1, copy_args);

  cudaMemcpy(dat->data_d, dat_d, dat->set->size * DG_NP * sizeof(double), cudaMemcpyDeviceToDevice);

  op_mpi_set_dirtybit_cuda(1, copy_args);
}

// Copy OP2 dat to PETSc vec array
void PETScUtils::copy_dat_to_vec(op_dat dat, double *dat_d) {
  op_arg copy_args[] = {
    op_arg_dat(dat, -1, OP_ID, DG_NP, "double", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(dat->set, 1, copy_args);

  cudaMemcpy(dat_d, dat->data_d , dat->set->size * DG_NP * sizeof(double), cudaMemcpyDeviceToDevice);

  op_mpi_set_dirtybit_cuda(1, copy_args);
}

// Create a PETSc vector for GPUs
void PETScUtils::create_vec(Vec *v, op_set set) {
  VecCreate(PETSC_COMM_WORLD, v);
  VecSetType(*v, VECCUDA);
  VecSetSizes(*v, set->size * DG_NP, PETSC_DECIDE);
}

// Destroy a PETSc vector
void PETScUtils::destroy_vec(Vec *v) {
  VecDestroy(v);
}

// Load a PETSc vector with values from an OP2 dat for GPUs
void PETScUtils::load_vec(Vec *v, op_dat v_dat) {
  double *v_ptr;
  VecCUDAGetArray(*v, &v_ptr);

  copy_dat_to_vec(v_dat, v_ptr);

  VecCUDARestoreArray(*v, &v_ptr);
}

// Load an OP2 dat with the values from a PETSc vector for GPUs
void PETScUtils::store_vec(Vec *v, op_dat v_dat) {
  const double *v_ptr;
  VecCUDAGetArrayRead(*v, &v_ptr);

  copy_vec_to_dat(v_dat, v_ptr);

  VecCUDARestoreArrayRead(*v, &v_ptr);
}

// P-adaptive stuff
// Copy PETSc vec array to OP2 dat
void PETScUtils::copy_vec_to_dat_p_adapt(op_dat dat, const double *dat_d, DGMesh2D *mesh) {
  op_arg copy_args[] = {
    op_arg_dat(dat, -1, OP_ID, DG_NP, "double", OP_WRITE),
    op_arg_dat(mesh->order, -1, OP_ID, 1, "int", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(dat->set, 2, copy_args);

  int setSize = dat->set->size;
  int *tempOrder = (int *)malloc(setSize * sizeof(int));
  cudaMemcpy(tempOrder, mesh->order->data_d, setSize * sizeof(int), cudaMemcpyDeviceToHost);

  int vec_ind = 0;
  int block_start = 0;
  int block_count = 0;
  for(int i = 0; i < setSize; i++) {
    const int N = tempOrder[i];

    if(N == DG_ORDER) {
      if(block_count == 0) {
        block_start = i;
        block_count++;
        continue;
      } else {
        block_count++;
        continue;
      }
    } else {
      if(block_count != 0) {
        double *block_start_dat_c = (double *)dat->data_d + block_start * dat->dim;
        cudaMemcpy(block_start_dat_c, dat_d + vec_ind, block_count * DG_NP * sizeof(double), cudaMemcpyDeviceToDevice);
        vec_ind += DG_NP * block_count;
      }
      block_count = 0;
    }

    double *v_c = (double *)dat->data_d + i * dat->dim;
    int Np, Nfp;
    DGUtils::numNodes2D(N, &Np, &Nfp);

    cudaMemcpy(v_c, dat_d + vec_ind, Np * sizeof(double), cudaMemcpyDeviceToDevice);
    vec_ind += Np;
  }

  if(block_count != 0) {
    double *block_start_dat_c = (double *)dat->data_d + block_start * dat->dim;
    cudaMemcpy(block_start_dat_c, dat_d + vec_ind, block_count * DG_NP * sizeof(double), cudaMemcpyDeviceToDevice);
    vec_ind += DG_NP * block_count;
  }
  free(tempOrder);
  op_mpi_set_dirtybit_cuda(2, copy_args);
}

// Copy OP2 dat to PETSc vec array
void PETScUtils::copy_dat_to_vec_p_adapt(op_dat dat, double *dat_d, DGMesh2D *mesh) {
  op_arg copy_args[] = {
    op_arg_dat(dat, -1, OP_ID, DG_NP, "double", OP_READ),
    op_arg_dat(mesh->order, -1, OP_ID, 1, "int", OP_READ)
  };
  op_mpi_halo_exchanges_cuda(dat->set, 2, copy_args);

  int setSize = dat->set->size;
  int *tempOrder = (int *)malloc(setSize * sizeof(int));
  cudaMemcpy(tempOrder, mesh->order->data_d, setSize * sizeof(int), cudaMemcpyDeviceToHost);

  int vec_ind = 0;
  int block_start = 0;
  int block_count = 0;
  for(int i = 0; i < setSize; i++) {
    const int N = tempOrder[i];

    if(N == DG_ORDER) {
      if(block_count == 0) {
        block_start = i;
        block_count++;
        continue;
      } else {
        block_count++;
        continue;
      }
    } else {
      if(block_count != 0) {
        const double *block_start_dat_c = (double *)dat->data_d + block_start * dat->dim;
        cudaMemcpy(dat_d + vec_ind, block_start_dat_c, block_count * DG_NP * sizeof(double), cudaMemcpyDeviceToDevice);
        vec_ind += DG_NP * block_count;
      }
      block_count = 0;
    }

    const double *v_c = (double *)dat->data_d + i * dat->dim;
    int Np, Nfp;
    DGUtils::numNodes2D(N, &Np, &Nfp);

    cudaMemcpy(dat_d + vec_ind, v_c, Np * sizeof(double), cudaMemcpyDeviceToDevice);
    vec_ind += Np;
  }

  if(block_count != 0) {
    const double *block_start_dat_c = (double *)dat->data_d + block_start * dat->dim;
    cudaMemcpy(dat_d + vec_ind, block_start_dat_c, block_count * DG_NP * sizeof(double), cudaMemcpyDeviceToDevice);
    vec_ind += DG_NP * block_count;
  }
  free(tempOrder);
  op_mpi_set_dirtybit_cuda(2, copy_args);
}

// Create a PETSc vector for GPUs
void PETScUtils::create_vec_p_adapt(Vec *v, int local_unknowns) {
  VecCreate(PETSC_COMM_WORLD, v);
  VecSetType(*v, VECCUDA);
  VecSetSizes(*v, local_unknowns, PETSC_DECIDE);
}