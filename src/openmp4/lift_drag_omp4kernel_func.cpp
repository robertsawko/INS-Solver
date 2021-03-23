//
// auto-generated by op2.py
//

void lift_drag_omp4_kernel(
  int *data0,
  int dat0size,
  int *data1,
  int dat1size,
  int *map2,
  int map2size,
  double *arg10,
  double *arg11,
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
  double *data8,
  int dat8size,
  double *data9,
  int dat9size,
  int *col_reord,
  int set_size1,
  int start,
  int end,
  int num_teams,
  int nthread){

  double arg10_l = *arg10;
  double arg11_l = *arg11;
  #pragma omp target teams num_teams(num_teams) thread_limit(nthread) map(to:data0[0:dat0size],data1[0:dat1size]) \
    map(to: nu_ompkernel, FMASK_ompkernel[:15], lift_drag_vec_ompkernel[:5])\
    map(to:col_reord[0:set_size1],map2[0:map2size],data2[0:dat2size],data3[0:dat3size],data4[0:dat4size],data5[0:dat5size],data6[0:dat6size],data7[0:dat7size],data8[0:dat8size],data9[0:dat9size])\
    map(tofrom: arg10_l, arg11_l) reduction(+:arg10_l) reduction(+:arg11_l)
  #pragma omp distribute parallel for schedule(static,1) reduction(+:arg10_l) reduction(+:arg11_l)
  for ( int e=start; e<end; e++ ){
    int n_op = col_reord[e];
    int map2idx;
    map2idx = map2[n_op + set_size1 * 0];

    //variable mapping
    const int *bedge_type = &data0[1*n_op];
    const int *bedgeNum = &data1[1*n_op];
    const double *p = &data2[15 * map2idx];
    const double *dQ0dx = &data3[15 * map2idx];
    const double *dQ0dy = &data4[15 * map2idx];
    const double *dQ1dx = &data5[15 * map2idx];
    const double *dQ1dy = &data6[15 * map2idx];
    const double *nx = &data7[15 * map2idx];
    const double *ny = &data8[15 * map2idx];
    const double *sJ = &data9[15 * map2idx];
    double *cd = &arg10_l;
    double *cl = &arg11_l;

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

    if(*bedge_type == 2) {
      for(int i = 0; i < 5; i++) {
        *cd += lift_drag_vec_ompkernel[i] * sJ[exInd + i] * (-p[fmask[i]] * nx[exInd + i] + nu_ompkernel * (nx[exInd + i] * 2.0 * dQ0dx[fmask[i]] + ny[exInd + i] * (dQ1dx[fmask[i]] + dQ0dy[fmask[i]])));
        *cl += lift_drag_vec_ompkernel[i] * sJ[exInd + i] * (-p[fmask[i]] * ny[exInd + i] + nu_ompkernel * (nx[exInd + i] * (dQ1dx[fmask[i]] + dQ0dy[fmask[i]]) + ny[exInd + i] * 2.0 * dQ1dy[fmask[i]]));
      }
    }
    //end inline func
  }

  *arg10 = arg10_l;
  *arg11 = arg11_l;
}