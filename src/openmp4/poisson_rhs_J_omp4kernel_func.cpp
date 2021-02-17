//
// auto-generated by op2.py
//

void poisson_rhs_J_omp4_kernel(
  double *data0,
  int dat0size,
  double *data1,
  int dat1size,
  int count,
  int num_teams,
  int nthread){

  #pragma omp target teams num_teams(num_teams) thread_limit(nthread) map(to:data0[0:dat0size],data1[0:dat1size])
  #pragma omp distribute parallel for schedule(static,1)
  for ( int n_op=0; n_op<count; n_op++ ){
    //variable mapping
    const double *J = &data0[15*n_op];
    double *rhs = &data1[15*n_op];

    //inline function
    
    for(int i = 0; i < 15; i++) {
      rhs[i] *= J[i];
    }
    //end inline func
  }

}