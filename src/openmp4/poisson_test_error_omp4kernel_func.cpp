//
// auto-generated by op2.py
//

void poisson_test_error_omp4_kernel(
  double *data0,
  int dat0size,
  double *data1,
  int dat1size,
  double *data2,
  int dat2size,
  double *data3,
  int dat3size,
  double *arg4,
  int count,
  int num_teams,
  int nthread){

  double arg4_l = *arg4;
  #pragma omp target teams num_teams(num_teams) thread_limit(nthread) map(to:data0[0:dat0size],data1[0:dat1size],data2[0:dat2size],data3[0:dat3size])\
    map(tofrom: arg4_l) reduction(+:arg4_l)
  #pragma omp distribute parallel for schedule(static,1) reduction(+:arg4_l)
  for ( int n_op=0; n_op<count; n_op++ ){
    //variable mapping
    const double *x = &data0[15*n_op];
    const double *y = &data1[15*n_op];
    const double *sol = &data2[15*n_op];
    double *err = &data3[15*n_op];
    double *l2 = &arg4_l;

    //inline function
    
    const double PI = 3.141592653589793238463;
    for(int i = 0; i < 15; i++) {
      double x1 = x[i];
      double y1 = y[i];



      double exact = y1 * (1.0 - y1) * x1 * x1 * x1;
      err[i] = fabs(sol[i] - exact);
      *l2 += err[i] * err[i];
    }
    //end inline func
  }

  *arg4 = arg4_l;
}
