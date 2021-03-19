//
// auto-generated by op2.py
//

//user function
//user function
//#pragma acc routine
inline void init_grid_openacc( const double **nc, double *nodeX, double *nodeY,
                      double *rx, double *ry, double *sx, double *sy,
                      double *nx, double *ny, double *J, double *sJ, double *fscale) {
  nodeX[0] = nc[0][0];
  nodeX[1] = nc[1][0];
  nodeX[2] = nc[2][0];
  nodeY[0] = nc[0][1];
  nodeY[1] = nc[1][1];
  nodeY[2] = nc[2][1];


  for(int i = 0; i < 5; i++) {
    nx[i] = ry[FMASK[i]];
    ny[i] = -rx[FMASK[i]];
  }

  for(int i = 0; i < 5; i++) {
    nx[5 + i] = sy[FMASK[5 + i]] - ry[FMASK[5 + i]];
    ny[5 + i] = rx[FMASK[5 + i]] - sx[FMASK[5 + i]];
  }

  for(int i = 0; i < 5; i++) {
    nx[2 * 5 + i] = -sy[FMASK[2 * 5 + i]];
    ny[2 * 5 + i] = sx[FMASK[2 * 5 + i]];
  }

  for(int i = 0; i < 15; i++) {
    J[i] = -sx[i] * ry[i] + rx[i] * sy[i];
  }

  for(int i = 0; i < 15; i++) {
    double rx_n = sy[i] / J[i];
    double sx_n = -ry[i] / J[i];
    double ry_n = -sx[i] / J[i];
    double sy_n = rx[i] / J[i];
    rx[i] = rx_n;
    sx[i] = sx_n;
    ry[i] = ry_n;
    sy[i] = sy_n;
  }

  for(int i = 0; i < 3 * 5; i++) {
    sJ[i] = sqrt(nx[i] * nx[i] + ny[i] * ny[i]);
    nx[i] = nx[i] / sJ[i];
    ny[i] = ny[i] / sJ[i];
    fscale[i] = sJ[i] / J[FMASK[i]];
  }
}

// host stub function
void op_par_loop_init_grid(char const *name, op_set set,
  op_arg arg0,
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
  op_arg arg13){

  int nargs = 14;
  op_arg args[14];

  arg0.idx = 0;
  args[0] = arg0;
  for ( int v=1; v<3; v++ ){
    args[0 + v] = op_arg_dat(arg0.dat, v, arg0.map, 2, "double", OP_READ);
  }

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

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(14);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[14].name      = name;
  OP_kernels[14].count    += 1;

  int  ninds   = 1;
  int  inds[14] = {0,0,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

  if (OP_diags>2) {
    printf(" kernel routine with indirection: init_grid\n");
  }

  // get plan
  #ifdef OP_PART_SIZE_14
    int part_size = OP_PART_SIZE_14;
  #else
    int part_size = OP_part_size;
  #endif

  int set_size = op_mpi_halo_exchanges_cuda(set, nargs, args);


  int ncolors = 0;

  if (set_size >0) {


    //Set up typed device pointers for OpenACC
    int *map0 = arg0.map_data_d;

    double* data3 = (double*)arg3.data_d;
    double* data4 = (double*)arg4.data_d;
    double* data5 = (double*)arg5.data_d;
    double* data6 = (double*)arg6.data_d;
    double* data7 = (double*)arg7.data_d;
    double* data8 = (double*)arg8.data_d;
    double* data9 = (double*)arg9.data_d;
    double* data10 = (double*)arg10.data_d;
    double* data11 = (double*)arg11.data_d;
    double* data12 = (double*)arg12.data_d;
    double* data13 = (double*)arg13.data_d;
    double *data0 = (double *)arg0.data_d;

    op_plan *Plan = op_plan_get_stage(name,set,part_size,nargs,args,ninds,inds,OP_COLOR2);
    ncolors = Plan->ncolors;
    int *col_reord = Plan->col_reord;
    int set_size1 = set->size + set->exec_size;

    // execute plan
    for ( int col=0; col<Plan->ncolors; col++ ){
      if (col==1) {
        op_mpi_wait_all_cuda(nargs, args);
      }
      int start = Plan->col_offsets[0][col];
      int end = Plan->col_offsets[0][col+1];

      #pragma acc parallel loop independent deviceptr(col_reord,map0,data3,data4,data5,data6,data7,data8,data9,data10,data11,data12,data13,data0)
      for ( int e=start; e<end; e++ ){
        int n = col_reord[e];
        int map0idx;
        int map1idx;
        int map2idx;
        map0idx = map0[n + set_size1 * 0];
        map1idx = map0[n + set_size1 * 1];
        map2idx = map0[n + set_size1 * 2];

        const double* arg0_vec[] = {
           &data0[2 * map0idx],
           &data0[2 * map1idx],
           &data0[2 * map2idx]};

        init_grid_openacc(
          arg0_vec,
          &data3[3 * n],
          &data4[3 * n],
          &data5[15 * n],
          &data6[15 * n],
          &data7[15 * n],
          &data8[15 * n],
          &data9[15 * n],
          &data10[15 * n],
          &data11[15 * n],
          &data12[15 * n],
          &data13[15 * n]);
      }

    }
    OP_kernels[14].transfer  += Plan->transfer;
    OP_kernels[14].transfer2 += Plan->transfer2;
  }

  if (set_size == 0 || set_size == set->core_size || ncolors == 1) {
    op_mpi_wait_all_cuda(nargs, args);
  }
  // combine reduction data
  op_mpi_set_dirtybit_cuda(nargs, args);

  // update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[14].time     += wall_t2 - wall_t1;
}
