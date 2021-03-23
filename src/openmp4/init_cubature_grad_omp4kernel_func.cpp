//
// auto-generated by op2.py
//

void init_cubature_grad_omp4_kernel(
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
  int count,
  int num_teams,
  int nthread){

  #pragma omp target teams num_teams(num_teams) thread_limit(nthread) map(to:data0[0:dat0size],data1[0:dat1size],data2[0:dat2size],data3[0:dat3size],data4[0:dat4size],data5[0:dat5size]) \
    map(to: cubVDr_ompkernel[:690], cubVDs_ompkernel[:690])
  #pragma omp distribute parallel for schedule(static,1)
  for ( int n_op=0; n_op<count; n_op++ ){
    //variable mapping
    double *rx = &data0[46*n_op];
    double *sx = &data1[46*n_op];
    double *ry = &data2[46*n_op];
    double *sy = &data3[46*n_op];
    double *Dx = &data4[690*n_op];
    double *Dy = &data5[690*n_op];

    //inline function
    

    double J[46];
    for(int i = 0; i < 46; i++) {
      J[i] = -sx[i] * ry[i] + rx[i] * sy[i];
    }

    for(int i = 0; i < 46; i++) {
      double rx_n = sy[i] / J[i];
      double sx_n = -ry[i] / J[i];
      double ry_n = -sx[i] / J[i];
      double sy_n = rx[i] / J[i];
      rx[i] = rx_n;
      sx[i] = sx_n;
      ry[i] = ry_n;
      sy[i] = sy_n;
    }

    for(int m = 0; m < 46; m++) {
      for(int n = 0; n < 15; n++) {
        int ind = m * 15 + n;
        Dx[ind] = rx[m] * cubVDr_ompkernel[ind] + sx[m] * cubVDs_ompkernel[ind];
        Dy[ind] = ry[m] * cubVDr_ompkernel[ind] + sy[m] * cubVDs_ompkernel[ind];
      }
    }
    //end inline func
  }

}