//
// auto-generated by op2.py
//

void poisson_rhs_qflux_omp4_kernel(
  double *data0,
  int dat0size,
  double *data1,
  int dat1size,
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
  int count,
  int num_teams,
  int nthread){

  #pragma omp target teams num_teams(num_teams) thread_limit(nthread) map(to:data0[0:dat0size],data1[0:dat1size],data2[0:dat2size],data3[0:dat3size],data4[0:dat4size],data5[0:dat5size],data6[0:dat6size],data7[0:dat7size]) \
    map(to: gaussW_g_ompkernel[:7])
  #pragma omp distribute parallel for schedule(static,1)
  for ( int n_op=0; n_op<count; n_op++ ){
    //variable mapping
    const double *nx = &data0[21*n_op];
    const double *ny = &data1[21*n_op];
    const double *sJ = &data2[21*n_op];
    const double *tau = &data3[3*n_op];
    double *uFlux = &data4[21*n_op];
    double *qxFlux = &data5[21*n_op];
    double *qyFlux = &data6[21*n_op];
    double *flux = &data7[21*n_op];

    //inline function
    
    for(int i = 0; i < 21; i++) {

      flux[i] = nx[i] * qxFlux[i] + ny[i] * qyFlux[i] + tau[i / 7] * (uFlux[i]);
      flux[i] *= gaussW_g_ompkernel[i % 7] * sJ[i];
    }

    for(int i = 0; i < 21; i++) {
      uFlux[i] = 0.0;
      qxFlux[i] = 0.0;
      qyFlux[i] = 0.0;
    }
    //end inline func
  }

}