//
// auto-generated by op2.py
//

void viscosity_bc_omp4_kernel(
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
  int *col_reord,
  int set_size1,
  int start,
  int end,
  int num_teams,
  int nthread){

  #pragma omp target teams num_teams(num_teams) thread_limit(nthread) map(to:data0[0:dat0size],data1[0:dat1size]) \
    map(to: FMASK_ompkernel[:15])\
    map(to:col_reord[0:set_size1],map2[0:map2size],data2[0:dat2size],data3[0:dat3size])
  #pragma omp distribute parallel for schedule(static,1)
  for ( int e=start; e<end; e++ ){
    int n_op = col_reord[e];
    int map2idx;
    map2idx = map2[n_op + set_size1 * 0];

    //variable mapping
    const int *bedge_type = &data0[1*n_op];
    const int *bedgeNum = &data1[1*n_op];
    double *vRHS0 = &data2[15 * map2idx];
    double *vRHS1 = &data3[15 * map2idx];

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
        vRHS0[exInd + i] += 1.0;

      }
    }
    //end inline func
  }

}
