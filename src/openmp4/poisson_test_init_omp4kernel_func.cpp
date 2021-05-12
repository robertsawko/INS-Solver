//
// auto-generated by op2.py
//

void poisson_test_init_omp4_kernel(
  double *data0,
  int dat0size,
  double *data1,
  int dat1size,
  double *data2,
  int dat2size,
  double *data3,
  int dat3size,
  int count,
  int num_teams,
  int nthread){

  #pragma omp target teams num_teams(num_teams) thread_limit(nthread) map(to:data0[0:dat0size],data1[0:dat1size],data2[0:dat2size],data3[0:dat3size])
  #pragma omp distribute parallel for schedule(static,1)
  for ( int n_op=0; n_op<count; n_op++ ){
    //variable mapping
    const double *x = &data0[15*n_op];
    const double *y = &data1[15*n_op];
    const double *J = &data2[15*n_op];
    double *rhs = &data3[15*n_op];

    //inline function
    
    for(int i = 0; i < 15; i++) {
      double x1 = x[i];
      double y1 = y[i];

      rhs[i] = 6 * x1 * y1 * (1.0 - y1) - 2.0 * x1 * x1 * x1;

      rhs[i] *= -J[i];
    }
    //end inline func
  }

}
