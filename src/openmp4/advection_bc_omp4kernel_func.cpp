//
// auto-generated by op2.py
//

void advection_bc_omp4_kernel(
  int *data0,
  int dat0size,
  int *data1,
  int dat1size,
  int *map2,
  int map2size,
  double *data2,
  int dat2size,
  double *data3,
  int dat3size,
  double *data4,
  int dat4size,
  double *data5,
  int dat5size,
  double *data6,
  int dat6size,
  double *data7,
  int dat7size,
  int *col_reord,
  int set_size1,
  int start,
  int end,
  int num_teams,
  int nthread){

  #pragma omp target teams num_teams(num_teams) thread_limit(nthread) map(to:data0[0:dat0size],data1[0:dat1size]) \
    map(to: bc_u_ompkernel, bc_v_ompkernel, FMASK_ompkernel[:15])\
    map(to:col_reord[0:set_size1],map2[0:map2size],data2[0:dat2size],data3[0:dat3size],data4[0:dat4size],data5[0:dat5size],data6[0:dat6size],data7[0:dat7size])
  #pragma omp distribute parallel for schedule(static,1)
  for ( int e=start; e<end; e++ ){
    int n_op = col_reord[e];
    int map2idx;
    map2idx = map2[n_op + set_size1 * 0];

    //variable mapping
    const int *bedge_type = &data0[1*n_op];
    const int *bedgeNum = &data1[1*n_op];
    const double *nx = &data2[15 * map2idx];
    const double *ny = &data3[15 * map2idx];
    const double *q0 = &data4[15 * map2idx];
    const double *q1 = &data5[15 * map2idx];
    double *exQ0 = &data6[15 * map2idx];
    double *exQ1 = &data7[15 * map2idx];

    //inline function
    
    int exInd = 0;
    if(*bedgeNum == 1) {
      exInd = 5;
    } else if(*bedgeNum == 2) {
      exInd = 2 * 5;
    }

    int *fmask;

    if(*bedgeNum == 0) {
      fmask = FMASK_ompkernel;
    } else if(*bedgeNum == 1) {
      fmask = &FMASK_ompkernel[5];
    } else {
      fmask = &FMASK_ompkernel[2 * 5];
    }

    if(*bedge_type == 0) {

      for(int i = 0; i < 5; i++) {
        exQ0[exInd + i] += bc_u_ompkernel;
        exQ1[exInd + i] += bc_v_ompkernel;
      }
    } else if(*bedge_type == 1) {

      for(int i = 0; i < 5; i++) {
        exQ0[exInd + i] += bc_u_ompkernel;
        exQ1[exInd + i] += bc_v_ompkernel;
      }
    } else {

      for(int i = 0; i < 5; i++) {
        int qInd = fmask[i];
        exQ0[exInd + i] += q0[qInd] - 2 * (nx[exInd + i] * q0[qInd] + ny[exInd + i] * q1[qInd]) * nx[exInd + i];
        exQ1[exInd + i] += q1[qInd] - 2 * (nx[exInd + i] * q0[qInd] + ny[exInd + i] * q1[qInd]) * ny[exInd + i];
      }
    }
    //end inline func
  }

}
