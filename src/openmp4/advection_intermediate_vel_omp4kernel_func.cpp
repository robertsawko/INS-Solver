//
// auto-generated by op2.py
//

void advection_intermediate_vel_omp4_kernel(
  double *arg0,
  double *arg1,
  double *arg2,
  double *arg3,
  double *arg4,
  double *arg5,
  double *data6,
  int dat6size,
  double *data7,
  int dat7size,
  double *data8,
  int dat8size,
  double *data9,
  int dat9size,
  double *data10,
  int dat10size,
  double *data11,
  int dat11size,
  double *data12,
  int dat12size,
  double *data13,
  int dat13size,
  double *data14,
  int dat14size,
  double *data15,
  int dat15size,
  int count,
  int num_teams,
  int nthread){

  double arg0_l = *arg0;
  double arg1_l = *arg1;
  double arg2_l = *arg2;
  double arg3_l = *arg3;
  double arg4_l = *arg4;
  double arg5_l = *arg5;
  #pragma omp target teams num_teams(num_teams) thread_limit(nthread) map(to:data6[0:dat6size],data7[0:dat7size],data8[0:dat8size],data9[0:dat9size],data10[0:dat10size],data11[0:dat11size],data12[0:dat12size],data13[0:dat13size],data14[0:dat14size],data15[0:dat15size])
  #pragma omp distribute parallel for schedule(static,1)
  for ( int n_op=0; n_op<count; n_op++ ){
    //variable mapping
    const double *a0 = &arg0_l;
    const double *a1 = &arg1_l;
    const double *b0 = &arg2_l;
    const double *b1 = &arg3_l;
    const double *g0 = &arg4_l;
    const double *dt = &arg5_l;
    const double *q0 = &data6[15*n_op];
    const double *q1 = &data7[15*n_op];
    const double *q0Old = &data8[15*n_op];
    const double *q1Old = &data9[15*n_op];
    const double *N0 = &data10[15*n_op];
    const double *N1 = &data11[15*n_op];
    const double *N0Old = &data12[15*n_op];
    const double *N1Old = &data13[15*n_op];
    double *q0T = &data14[15*n_op];
    double *q1T = &data15[15*n_op];

    //inline function
    
    for(int i = 0; i < 15; i++) {
      q0T[i] = ((*a0 * q0[i] + *a1 * q0Old[i]) - *dt * (*b0 * N0[i] + *b1 * N0Old[i])) / *g0;
      q1T[i] = ((*a0 * q1[i] + *a1 * q1Old[i]) - *dt * (*b0 * N1[i] + *b1 * N1Old[i])) / *g0;
    }
    //end inline func
  }

  *arg0 = arg0_l;
  *arg1 = arg1_l;
  *arg2 = arg2_l;
  *arg3 = arg3_l;
  *arg4 = arg4_l;
  *arg5 = arg5_l;
}