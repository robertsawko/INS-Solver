#include "op_lib_cpp.h"
#include "op_cuda_rt_support.h"
#include "op_cuda_reduction.h"

#include "dg_compiler_defs.h"

template<int p, int dg_np, int dg_npf>
__device__ void _pmf_3d_mult_cells_part1_gpu(const int ind, const double *mMat, const double *eMat,
                      const double *J, const double *lx, const double *ly, const double *lz,
                      const double *out_tmp, const double *ux, const double *uy, const double *uz,
                      double *outx, double *outy, double *outz, double *out) {
  const double *mmat_mat = &mMat[(p - 1) * DG_NP * DG_NP];
  const double *emat_mat = &eMat[(p - 1) * DG_NUM_FACES * DG_NPF * DG_NP];

  double outx_t[4] = {0.0, 0.0, 0.0, 0.0};
  double outy_t[4] = {0.0, 0.0, 0.0, 0.0};
  double outz_t[4] = {0.0, 0.0, 0.0, 0.0};
  for(int j = 0; j < dg_np; j++) {
    const DG_FP ux_tmp = ux[j];
    const DG_FP uy_tmp = uy[j];
    const DG_FP uz_tmp = uz[j];
    for(int i = 0; i < 4; i++) {
      int mat_ind = DG_MAT_IND(ind + i, j, dg_np, dg_np);
      outx_t[i] += mmat_mat[mat_ind] * ux_tmp;
      outy_t[i] += mmat_mat[mat_ind] * uy_tmp;
      outz_t[i] += mmat_mat[mat_ind] * uz_tmp;
    }
  }
  for(int i = 0; i < 4; i++) {
    outx_t[i] *= *J;
    outy_t[i] *= *J;
    outz_t[i] *= *J;
  }
  double out_t[4] = {0.0, 0.0, 0.0, 0.0};
  for(int j = 0; j < DG_NUM_FACES * dg_npf; j++) {
    const DG_FP lx_tmp = lx[j];
    const DG_FP ly_tmp = ly[j];
    const DG_FP lz_tmp = lz[j];
    const DG_FP _out_tmp = out_tmp[j];
    for(int i = 0; i < 4; i++) {
      int mat_ind = DG_MAT_IND(ind + i, j, dg_np, DG_NUM_FACES * dg_npf);
      outx_t[i] += emat_mat[mat_ind] * lx_tmp;
      outy_t[i] += emat_mat[mat_ind] * ly_tmp;
      outz_t[i] += emat_mat[mat_ind] * lz_tmp;
      out_t[i]  += emat_mat[mat_ind] * _out_tmp;
    }
  }

  for(int i = 0; i < 4; i++) {
    outx[ind + i] = outx_t[i];
    outy[ind + i] = outy_t[i];
    outz[ind + i] = outz_t[i];
    out[ind + i]  = out_t[i];
  }
}

template<int p, int dg_np>
__device__ void _pmf_3d_mult_cells_part2_gpu(const int ind, const double *dr,
                            const double *ds, const double *dt,
                            const double *in_r, const double *in_s,
                            const double *in_t, double *out) {
  const double *dr_mat = &dr[(p - 1) * DG_NP * DG_NP];
  const double *ds_mat = &ds[(p - 1) * DG_NP * DG_NP];
  const double *dt_mat = &dt[(p - 1) * DG_NP * DG_NP];

  double tmp[4] = {0.0, 0.0, 0.0, 0.0};
  for(int n = 0; n < dg_np; n++) {
    const DG_FP r_tmp = in_r[n];
    const DG_FP s_tmp = in_s[n];
    const DG_FP t_tmp = in_t[n];
    for(int i = 0; i < 4; i++) {
      int mat_ind = DG_MAT_IND(n, ind + i, dg_np, dg_np);
      tmp[i] += dr_mat[mat_ind] * r_tmp;
      tmp[i] += ds_mat[mat_ind] * s_tmp;
      tmp[i] += dt_mat[mat_ind] * t_tmp;
    }
  }
  for(int i = 0; i < 4; i++) {
    out[ind + i] += tmp[i];
  }
}

// CUDA kernel function
template<int p, int NUM_CELLS>
__global__ void _op_cuda_pmf_3d_mult_cells_merged(
  const int *__restrict arg0,
  const double *arg1,
  const double *arg2,
  const double *arg3,
  const double *arg4,
  const double *arg5,
  const double *__restrict arg6,
  const double *__restrict arg7,
  const double *__restrict arg8,
  const double *__restrict arg9,
  const double *__restrict arg10,
  const double *__restrict arg11,
  const double *__restrict arg12,
  const double *__restrict arg13,
  const double *__restrict arg14,
  const double *__restrict arg15,
  const double *__restrict arg16,
  const double *__restrict arg17,
  const double *__restrict arg18,
  const double *__restrict arg19,
  const double *__restrict arg20,
  const double *__restrict arg21,
  const double *__restrict arg22,
  double *arg23,
  int   set_size ) {
  const int np = (p + 1) * (p + 2) * (p + 3) / 6;
  const int np_4 = np / 4;
  const int npf = (p + 1) * (p + 2) / 2;

  __shared__ double ux_shared[NUM_CELLS * np];
  __shared__ double uy_shared[NUM_CELLS * np];
  __shared__ double uz_shared[NUM_CELLS * np];

  __shared__ double lx_shared[NUM_CELLS * npf * 4];
  __shared__ double ly_shared[NUM_CELLS * npf * 4];
  __shared__ double lz_shared[NUM_CELLS * npf * 4];
  __shared__ double lo_shared[NUM_CELLS * npf * 4];

  __shared__ double tmp_x_shared[NUM_CELLS * np];
  __shared__ double tmp_y_shared[NUM_CELLS * np];
  __shared__ double tmp_z_shared[NUM_CELLS * np];

  //process set elements
  for (int n = threadIdx.x + blockIdx.x * blockDim.x; n - threadIdx.x < set_size * np_4; n += blockDim.x * gridDim.x){
    const int node_id = (n % np_4) * 4;
    const int cell_id = n / np_4;
    const int local_cell_id = (n / np_4) - ((n - threadIdx.x) / np_4);
    const int start_ind = ((n - threadIdx.x) / np_4) * DG_NP;
    const int num_elem  = min((n - threadIdx.x + blockDim.x) / np_4, set_size) - ((n - threadIdx.x) / np_4) + 1;
    for(int i = threadIdx.x; i < num_elem * np; i += blockDim.x) {
      int curr_cell = i / np + (n - threadIdx.x) / np_4;
      int curr_node = i % np;
      ux_shared[i] = arg20[curr_cell * DG_NP + curr_node];
      uy_shared[i] = arg21[curr_cell * DG_NP + curr_node];
      uz_shared[i] = arg22[curr_cell * DG_NP + curr_node];
    }
    for(int i = threadIdx.x; i < num_elem * npf * 4; i += blockDim.x) {
      int curr_cell = i / (npf * 4) + (n - threadIdx.x) / np_4;
      int curr_node = i % (npf * 4);
      lx_shared[i] = arg16[curr_cell * DG_NPF * 4 + curr_node];
      ly_shared[i] = arg17[curr_cell * DG_NPF * 4 + curr_node];
      lz_shared[i] = arg18[curr_cell * DG_NPF * 4 + curr_node];
      lo_shared[i] = arg19[curr_cell * DG_NPF * 4 + curr_node];
    }
    __syncthreads();
    //user-supplied kernel call
    if(n < set_size * np_4)
      _pmf_3d_mult_cells_part1_gpu<p,np,npf>(node_id,
                               arg2,
                               arg1,
                               arg15 + cell_id,
                               lx_shared + local_cell_id * DG_NUM_FACES * npf,
                               ly_shared + local_cell_id * DG_NUM_FACES * npf,
                               lz_shared + local_cell_id * DG_NUM_FACES * npf,
                               lo_shared + local_cell_id * DG_NUM_FACES * npf,
                               ux_shared + local_cell_id * np,
                               uy_shared + local_cell_id * np,
                               uz_shared + local_cell_id * np,
                               tmp_x_shared + local_cell_id * np,
                               tmp_y_shared + local_cell_id * np,
                               tmp_z_shared + local_cell_id * np,
                               arg23 + cell_id * DG_NP);
    __syncthreads();
    for(int i = threadIdx.x; i < num_elem * np; i += blockDim.x) {
      int curr_cell = i / np + (n - threadIdx.x) / np_4;
      DG_FP tmp_x = tmp_x_shared[i];
      DG_FP tmp_y = tmp_y_shared[i];
      DG_FP tmp_z = tmp_z_shared[i];
      tmp_x_shared[i] = *(arg6 + curr_cell) * tmp_x + *(arg9 + curr_cell) * tmp_y + *(arg12 + curr_cell) * tmp_z;
      tmp_y_shared[i] = *(arg7 + curr_cell) * tmp_x + *(arg10 + curr_cell) * tmp_y + *(arg13 + curr_cell) * tmp_z;
      tmp_z_shared[i] = *(arg8 + curr_cell) * tmp_x + *(arg11 + curr_cell) * tmp_y + *(arg14 + curr_cell) * tmp_z;
    }
    __syncthreads();
    if(n < set_size * np_4)
      _pmf_3d_mult_cells_part2_gpu<p,np>(node_id, arg3, arg4, arg5,
                                  tmp_x_shared + local_cell_id * np,
                                  tmp_y_shared + local_cell_id * np,
                                  tmp_z_shared + local_cell_id * np,
                                  arg23 + cell_id * DG_NP);
  }
}

#include <stdexcept>
#include "timing.h"
extern Timing *timer;

//host stub function
void custom_kernel_pmf_3d_mult_cells_merged(const int order, char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2,
  op_arg arg3,
  op_arg arg4,
  op_arg arg5,
  op_arg arg6,
  op_arg arg7,
  op_arg arg8,
  op_arg arg9,
  op_arg arg10,
  op_arg arg11,
  op_arg arg12,
  op_arg arg13,
  op_arg arg14,
  op_arg arg15,
  op_arg arg16,
  op_arg arg17,
  op_arg arg18,
  op_arg arg19,
  op_arg arg20,
  op_arg arg21,
  op_arg arg22,
  op_arg arg23){

  double*arg1h = (double *)arg1.data;
  double*arg2h = (double *)arg2.data;
  double*arg3h = (double *)arg3.data;
  double*arg4h = (double *)arg4.data;
  double*arg5h = (double *)arg5.data;
  int nargs = 24;
  op_arg args[24];

  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;
  args[3] = arg3;
  args[4] = arg4;
  args[5] = arg5;
  args[6] = arg6;
  args[7] = arg7;
  args[8] = arg8;
  args[9] = arg9;
  args[10] = arg10;
  args[11] = arg11;
  args[12] = arg12;
  args[13] = arg13;
  args[14] = arg14;
  args[15] = arg15;
  args[16] = arg16;
  args[17] = arg17;
  args[18] = arg18;
  args[19] = arg19;
  args[20] = arg20;
  args[21] = arg21;
  args[22] = arg22;
  args[23] = arg23;

  if (OP_diags>2) {
    printf(" kernel routine w/o indirection:  pmf_3d_mult_cells_merged");
  }

  int set_size = op_mpi_halo_exchanges_grouped(set, nargs, args, 2);
  if (set_size > 0) {

    //transfer constants to GPU
    int consts_bytes = 0;
    consts_bytes += ROUND_UP(DG_ORDER * DG_NUM_FACES * DG_NPF * DG_NP *sizeof(double));
    consts_bytes += ROUND_UP(DG_ORDER * DG_NP * DG_NP *sizeof(double));
    consts_bytes += ROUND_UP(DG_ORDER * DG_NP * DG_NP *sizeof(double));
    consts_bytes += ROUND_UP(DG_ORDER * DG_NP * DG_NP *sizeof(double));
    consts_bytes += ROUND_UP(DG_ORDER * DG_NP * DG_NP *sizeof(double));
    reallocConstArrays(consts_bytes);
    consts_bytes = 0;
    arg1.data   = OP_consts_h + consts_bytes;
    arg1.data_d = OP_consts_d + consts_bytes;
    memcpy(arg1.data, arg1h, DG_ORDER * DG_NUM_FACES * DG_NPF * DG_NP * sizeof(double));
    consts_bytes += ROUND_UP(DG_ORDER * DG_NUM_FACES * DG_NPF * DG_NP * sizeof(double));
    arg2.data   = OP_consts_h + consts_bytes;
    arg2.data_d = OP_consts_d + consts_bytes;
    memcpy(arg2.data, arg2h, DG_ORDER * DG_NP * DG_NP * sizeof(double));
    consts_bytes += ROUND_UP(DG_ORDER * DG_NP * DG_NP * sizeof(double));
    arg3.data   = OP_consts_h + consts_bytes;
    arg3.data_d = OP_consts_d + consts_bytes;
    memcpy(arg3.data, arg3h, DG_ORDER * DG_NP * DG_NP * sizeof(double));
    consts_bytes += ROUND_UP(DG_ORDER * DG_NP * DG_NP * sizeof(double));
    arg4.data   = OP_consts_h + consts_bytes;
    arg4.data_d = OP_consts_d + consts_bytes;
    memcpy(arg4.data, arg4h, DG_ORDER * DG_NP * DG_NP * sizeof(double));
    consts_bytes += ROUND_UP(DG_ORDER * DG_NP * DG_NP * sizeof(double));
    arg5.data   = OP_consts_h + consts_bytes;
    arg5.data_d = OP_consts_d + consts_bytes;
    memcpy(arg5.data, arg5h, DG_ORDER * DG_NP * DG_NP * sizeof(double));
    consts_bytes += ROUND_UP(DG_ORDER * DG_NP * DG_NP * sizeof(double));
    mvConstArraysToDevice(consts_bytes);

    //set CUDA execution parameters
    const int np  = (order + 1) * (order + 2) * (order + 3) / 6;
    const int np_4  = np / 4;
    const int nthread = (64 /  np_4) * np_4;
    const int nblocks = 200 < (set->size * np_4) / nthread + 1 ? 200 : (set->size * np_4) / nthread + 1;
    // const int num_cells = (nthread / DG_NP) + 1;

    if(np % 4 != 0)
      throw std::runtime_error("Merged cells kernel has not been implemented for NP not divisible by 4\n");

    switch(order) {
      case 1:
        _op_cuda_pmf_3d_mult_cells_merged<1,64 + 1><<<nblocks,nthread>>>(
          (int *) arg0.data_d,
          (double *) arg1.data_d,
          (double *) arg2.data_d,
          (double *) arg3.data_d,
          (double *) arg4.data_d,
          (double *) arg5.data_d,
          (double *) arg6.data_d,
          (double *) arg7.data_d,
          (double *) arg8.data_d,
          (double *) arg9.data_d,
          (double *) arg10.data_d,
          (double *) arg11.data_d,
          (double *) arg12.data_d,
          (double *) arg13.data_d,
          (double *) arg14.data_d,
          (double *) arg15.data_d,
          (double *) arg16.data_d,
          (double *) arg17.data_d,
          (double *) arg18.data_d,
          (double *) arg19.data_d,
          (double *) arg20.data_d,
          (double *) arg21.data_d,
          (double *) arg22.data_d,
          (double *) arg23.data_d,
          set->size );
        break;
      case 2:
        _op_cuda_pmf_3d_mult_cells_merged<2,(64 / 10) + 1><<<nblocks,nthread>>>(
          (int *) arg0.data_d,
          (double *) arg1.data_d,
          (double *) arg2.data_d,
          (double *) arg3.data_d,
          (double *) arg4.data_d,
          (double *) arg5.data_d,
          (double *) arg6.data_d,
          (double *) arg7.data_d,
          (double *) arg8.data_d,
          (double *) arg9.data_d,
          (double *) arg10.data_d,
          (double *) arg11.data_d,
          (double *) arg12.data_d,
          (double *) arg13.data_d,
          (double *) arg14.data_d,
          (double *) arg15.data_d,
          (double *) arg16.data_d,
          (double *) arg17.data_d,
          (double *) arg18.data_d,
          (double *) arg19.data_d,
          (double *) arg20.data_d,
          (double *) arg21.data_d,
          (double *) arg22.data_d,
          (double *) arg23.data_d,
          set->size );
        break;
      case 3:
        timer->startTimer("fpmf_cells_merged 3rd order");
        _op_cuda_pmf_3d_mult_cells_merged<3,(64 / 5) + 1><<<nblocks,nthread>>>(
          (int *) arg0.data_d,
          (double *) arg1.data_d,
          (double *) arg2.data_d,
          (double *) arg3.data_d,
          (double *) arg4.data_d,
          (double *) arg5.data_d,
          (double *) arg6.data_d,
          (double *) arg7.data_d,
          (double *) arg8.data_d,
          (double *) arg9.data_d,
          (double *) arg10.data_d,
          (double *) arg11.data_d,
          (double *) arg12.data_d,
          (double *) arg13.data_d,
          (double *) arg14.data_d,
          (double *) arg15.data_d,
          (double *) arg16.data_d,
          (double *) arg17.data_d,
          (double *) arg18.data_d,
          (double *) arg19.data_d,
          (double *) arg20.data_d,
          (double *) arg21.data_d,
          (double *) arg22.data_d,
          (double *) arg23.data_d,
          set->size );
        cutilSafeCall(cudaDeviceSynchronize());
        timer->endTimer("fpmf_cells_merged 3rd order");
        break;
      case 4:
        _op_cuda_pmf_3d_mult_cells_merged<4,(64 / 35) + 1><<<nblocks,nthread>>>(
          (int *) arg0.data_d,
          (double *) arg1.data_d,
          (double *) arg2.data_d,
          (double *) arg3.data_d,
          (double *) arg4.data_d,
          (double *) arg5.data_d,
          (double *) arg6.data_d,
          (double *) arg7.data_d,
          (double *) arg8.data_d,
          (double *) arg9.data_d,
          (double *) arg10.data_d,
          (double *) arg11.data_d,
          (double *) arg12.data_d,
          (double *) arg13.data_d,
          (double *) arg14.data_d,
          (double *) arg15.data_d,
          (double *) arg16.data_d,
          (double *) arg17.data_d,
          (double *) arg18.data_d,
          (double *) arg19.data_d,
          (double *) arg20.data_d,
          (double *) arg21.data_d,
          (double *) arg22.data_d,
          (double *) arg23.data_d,
          set->size );
        break;
      case 5:
        _op_cuda_pmf_3d_mult_cells_merged<5,(64 / 56) + 1><<<nblocks,nthread>>>(
          (int *) arg0.data_d,
          (double *) arg1.data_d,
          (double *) arg2.data_d,
          (double *) arg3.data_d,
          (double *) arg4.data_d,
          (double *) arg5.data_d,
          (double *) arg6.data_d,
          (double *) arg7.data_d,
          (double *) arg8.data_d,
          (double *) arg9.data_d,
          (double *) arg10.data_d,
          (double *) arg11.data_d,
          (double *) arg12.data_d,
          (double *) arg13.data_d,
          (double *) arg14.data_d,
          (double *) arg15.data_d,
          (double *) arg16.data_d,
          (double *) arg17.data_d,
          (double *) arg18.data_d,
          (double *) arg19.data_d,
          (double *) arg20.data_d,
          (double *) arg21.data_d,
          (double *) arg22.data_d,
          (double *) arg23.data_d,
          set->size );
        break;
    }
  }
  op_mpi_set_dirtybit_cuda(nargs, args);
  cutilSafeCall(cudaDeviceSynchronize());
}
