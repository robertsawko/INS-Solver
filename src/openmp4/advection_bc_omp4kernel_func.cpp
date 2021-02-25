//
// auto-generated by op2.py
//

void advection_bc_omp4_kernel(
  int *data0,
  int dat0size,
  int *data1,
  int dat1size,
  double *arg2,
  int *map3,
  int map3size,
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
  int *col_reord,
  int set_size1,
  int start,
  int end,
  int num_teams,
  int nthread){

  double arg2_l = *arg2;
  #pragma omp target teams num_teams(num_teams) thread_limit(nthread) map(to:data0[0:dat0size],data1[0:dat1size]) \
    map(to: FMASK_ompkernel[:15])\
    map(to:col_reord[0:set_size1],map3[0:map3size],data3[0:dat3size],data4[0:dat4size],data5[0:dat5size],data6[0:dat6size],data7[0:dat7size],data8[0:dat8size])
  #pragma omp distribute parallel for schedule(static,1)
  for ( int e=start; e<end; e++ ){
    int n_op = col_reord[e];
    int map3idx;
    map3idx = map3[n_op + set_size1 * 0];

    //variable mapping
    const int *bedge_type = &data0[1*n_op];
    const int *bedgeNum = &data1[1*n_op];
    const double *t = &arg2_l;
    const double *x = &data3[15 * map3idx];
    const double *y = &data4[15 * map3idx];
    const double *q0 = &data5[15 * map3idx];
    const double *q1 = &data6[15 * map3idx];
    double *exQ0 = &data7[15 * map3idx];
    double *exQ1 = &data8[15 * map3idx];

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

      const double PI = 3.141592653589793238463;
      for(int i = 0; i < 5; i++) {
        int qInd = fmask[i];
        double y1 = y[qInd];
        exQ0[exInd + i] += pow(0.41, -2.0) * sin((PI * *t) / 8.0) * 6.0 * y1 * (0.41 - y1);



      }
    } else if(*bedge_type == 1) {

      for(int i = 0; i < 5; i++) {
        int qInd = fmask[i];
        exQ0[exInd + i] += q0[qInd];
        exQ1[exInd + i] += q1[qInd];
      }
    } else {

      for(int i = 0; i < 5; i++) {



      }
    }
    //end inline func
  }

  *arg2 = arg2_l;
}
