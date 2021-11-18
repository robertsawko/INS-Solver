//
// auto-generated by op2.py
//

//user function
__device__ void poisson_mf2_opf_gpu( const double *tol, const int *edgeNum, const double *gop0L,
                            const double *gop1L, const double *gop2L, const double *gopf0L,
                            const double *gopf1L, const double *gopf2L, double *op2L,
                            double *op1L,
                            const double *gop0R, const double *gop1R, const double *gop2R,
                            const double *gopf0R, const double *gopf1R, const double *gopf2R,
                            double *op2R, double *op1R) {
  int edgeL = edgeNum[0];
  int edgeR = edgeNum[1];

  if(edgeL == 0) {
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int ind = m * 15 + n;
        int colInd = n * 15 + m;
        double val = 0.5 * gop0L[colInd];
        if(fabs(val) > *tol) {
          op1L[ind] += val;
        }
        val = -0.5 * gopf0L[colInd];
        if(fabs(val) > *tol) {
          op2L[ind] += val;
        }
      }
    }
  } else if(edgeL == 1) {
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int ind = m * 15 + n;
        int colInd = n * 15 + m;
        double val = 0.5 * gop1L[colInd];
        if(fabs(val) > *tol) {
          op1L[ind] += val;
        }
        val = -0.5 * gopf1L[colInd];
        if(fabs(val) > *tol) {
          op2L[ind] += val;
        }
      }
    }
  } else {
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int ind = m * 15 + n;
        int colInd = n * 15 + m;
        double val = 0.5 * gop2L[colInd];
        if(fabs(val) > *tol) {
          op1L[ind] += val;
        }
        val = -0.5 * gopf2L[colInd];
        if(fabs(val) > *tol) {
          op2L[ind] += val;
        }
      }
    }
  }

  if(edgeR == 0) {
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int ind = m * 15 + n;
        int colInd = n * 15 + m;
        double val = 0.5 * gop0R[colInd];
        if(fabs(val) > *tol) {
          op1R[ind] += val;
        }
        val = -0.5 * gopf0R[colInd];
        if(fabs(val) > *tol) {
          op2R[ind] += val;
        }
      }
    }
  } else if(edgeR == 1) {
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int ind = m * 15 + n;
        int colInd = n * 15 + m;
        double val = 0.5 * gop1R[colInd];
        if(fabs(val) > *tol) {
          op1R[ind] += val;
        }
        val = -0.5 * gopf1R[colInd];
        if(fabs(val) > *tol) {
          op2R[ind] += val;
        }
      }
    }
  } else {
    for(int m = 0; m < 15; m++) {
      for(int n = 0; n < 15; n++) {
        int ind = m * 15 + n;
        int colInd = n * 15 + m;
        double val = 0.5 * gop2R[colInd];
        if(fabs(val) > *tol) {
          op1R[ind] += val;
        }
        val = -0.5 * gopf2R[colInd];
        if(fabs(val) > *tol) {
          op2R[ind] += val;
        }
      }
    }
  }

}

// CUDA kernel function
__global__ void op_cuda_poisson_mf2_opf(
  const double *__restrict ind_arg0,
  const double *__restrict ind_arg1,
  const double *__restrict ind_arg2,
  const double *__restrict ind_arg3,
  const double *__restrict ind_arg4,
  const double *__restrict ind_arg5,
  double *__restrict ind_arg6,
  const int *__restrict opDat2Map,
  const double *arg0,
  const int *__restrict arg1,
  double *arg8,
  double *arg16,
  int start,
  int end,
  int   set_size) {
  int tid = threadIdx.x + blockIdx.x * blockDim.x;
  if (tid + start < end) {
    int n = tid + start;
    //initialise local variables
    double arg9_l[225];
    for ( int d=0; d<225; d++ ){
      arg9_l[d] = ZERO_double;
    }
    double arg17_l[225];
    for ( int d=0; d<225; d++ ){
      arg17_l[d] = ZERO_double;
    }
    int map2idx;
    int map10idx;
    map2idx = opDat2Map[n + set_size * 0];
    map10idx = opDat2Map[n + set_size * 1];

    //user-supplied kernel call
    poisson_mf2_opf_gpu(arg0,
                    arg1+n*2,
                    ind_arg0+map2idx*225,
                    ind_arg1+map2idx*225,
                    ind_arg2+map2idx*225,
                    ind_arg3+map2idx*225,
                    ind_arg4+map2idx*225,
                    ind_arg5+map2idx*225,
                    arg8+n*225,
                    arg9_l,
                    ind_arg0+map10idx*225,
                    ind_arg1+map10idx*225,
                    ind_arg2+map10idx*225,
                    ind_arg3+map10idx*225,
                    ind_arg4+map10idx*225,
                    ind_arg5+map10idx*225,
                    arg16+n*225,
                    arg17_l);
    atomicAdd(&ind_arg6[0+map2idx*225],arg9_l[0]);
    atomicAdd(&ind_arg6[1+map2idx*225],arg9_l[1]);
    atomicAdd(&ind_arg6[2+map2idx*225],arg9_l[2]);
    atomicAdd(&ind_arg6[3+map2idx*225],arg9_l[3]);
    atomicAdd(&ind_arg6[4+map2idx*225],arg9_l[4]);
    atomicAdd(&ind_arg6[5+map2idx*225],arg9_l[5]);
    atomicAdd(&ind_arg6[6+map2idx*225],arg9_l[6]);
    atomicAdd(&ind_arg6[7+map2idx*225],arg9_l[7]);
    atomicAdd(&ind_arg6[8+map2idx*225],arg9_l[8]);
    atomicAdd(&ind_arg6[9+map2idx*225],arg9_l[9]);
    atomicAdd(&ind_arg6[10+map2idx*225],arg9_l[10]);
    atomicAdd(&ind_arg6[11+map2idx*225],arg9_l[11]);
    atomicAdd(&ind_arg6[12+map2idx*225],arg9_l[12]);
    atomicAdd(&ind_arg6[13+map2idx*225],arg9_l[13]);
    atomicAdd(&ind_arg6[14+map2idx*225],arg9_l[14]);
    atomicAdd(&ind_arg6[15+map2idx*225],arg9_l[15]);
    atomicAdd(&ind_arg6[16+map2idx*225],arg9_l[16]);
    atomicAdd(&ind_arg6[17+map2idx*225],arg9_l[17]);
    atomicAdd(&ind_arg6[18+map2idx*225],arg9_l[18]);
    atomicAdd(&ind_arg6[19+map2idx*225],arg9_l[19]);
    atomicAdd(&ind_arg6[20+map2idx*225],arg9_l[20]);
    atomicAdd(&ind_arg6[21+map2idx*225],arg9_l[21]);
    atomicAdd(&ind_arg6[22+map2idx*225],arg9_l[22]);
    atomicAdd(&ind_arg6[23+map2idx*225],arg9_l[23]);
    atomicAdd(&ind_arg6[24+map2idx*225],arg9_l[24]);
    atomicAdd(&ind_arg6[25+map2idx*225],arg9_l[25]);
    atomicAdd(&ind_arg6[26+map2idx*225],arg9_l[26]);
    atomicAdd(&ind_arg6[27+map2idx*225],arg9_l[27]);
    atomicAdd(&ind_arg6[28+map2idx*225],arg9_l[28]);
    atomicAdd(&ind_arg6[29+map2idx*225],arg9_l[29]);
    atomicAdd(&ind_arg6[30+map2idx*225],arg9_l[30]);
    atomicAdd(&ind_arg6[31+map2idx*225],arg9_l[31]);
    atomicAdd(&ind_arg6[32+map2idx*225],arg9_l[32]);
    atomicAdd(&ind_arg6[33+map2idx*225],arg9_l[33]);
    atomicAdd(&ind_arg6[34+map2idx*225],arg9_l[34]);
    atomicAdd(&ind_arg6[35+map2idx*225],arg9_l[35]);
    atomicAdd(&ind_arg6[36+map2idx*225],arg9_l[36]);
    atomicAdd(&ind_arg6[37+map2idx*225],arg9_l[37]);
    atomicAdd(&ind_arg6[38+map2idx*225],arg9_l[38]);
    atomicAdd(&ind_arg6[39+map2idx*225],arg9_l[39]);
    atomicAdd(&ind_arg6[40+map2idx*225],arg9_l[40]);
    atomicAdd(&ind_arg6[41+map2idx*225],arg9_l[41]);
    atomicAdd(&ind_arg6[42+map2idx*225],arg9_l[42]);
    atomicAdd(&ind_arg6[43+map2idx*225],arg9_l[43]);
    atomicAdd(&ind_arg6[44+map2idx*225],arg9_l[44]);
    atomicAdd(&ind_arg6[45+map2idx*225],arg9_l[45]);
    atomicAdd(&ind_arg6[46+map2idx*225],arg9_l[46]);
    atomicAdd(&ind_arg6[47+map2idx*225],arg9_l[47]);
    atomicAdd(&ind_arg6[48+map2idx*225],arg9_l[48]);
    atomicAdd(&ind_arg6[49+map2idx*225],arg9_l[49]);
    atomicAdd(&ind_arg6[50+map2idx*225],arg9_l[50]);
    atomicAdd(&ind_arg6[51+map2idx*225],arg9_l[51]);
    atomicAdd(&ind_arg6[52+map2idx*225],arg9_l[52]);
    atomicAdd(&ind_arg6[53+map2idx*225],arg9_l[53]);
    atomicAdd(&ind_arg6[54+map2idx*225],arg9_l[54]);
    atomicAdd(&ind_arg6[55+map2idx*225],arg9_l[55]);
    atomicAdd(&ind_arg6[56+map2idx*225],arg9_l[56]);
    atomicAdd(&ind_arg6[57+map2idx*225],arg9_l[57]);
    atomicAdd(&ind_arg6[58+map2idx*225],arg9_l[58]);
    atomicAdd(&ind_arg6[59+map2idx*225],arg9_l[59]);
    atomicAdd(&ind_arg6[60+map2idx*225],arg9_l[60]);
    atomicAdd(&ind_arg6[61+map2idx*225],arg9_l[61]);
    atomicAdd(&ind_arg6[62+map2idx*225],arg9_l[62]);
    atomicAdd(&ind_arg6[63+map2idx*225],arg9_l[63]);
    atomicAdd(&ind_arg6[64+map2idx*225],arg9_l[64]);
    atomicAdd(&ind_arg6[65+map2idx*225],arg9_l[65]);
    atomicAdd(&ind_arg6[66+map2idx*225],arg9_l[66]);
    atomicAdd(&ind_arg6[67+map2idx*225],arg9_l[67]);
    atomicAdd(&ind_arg6[68+map2idx*225],arg9_l[68]);
    atomicAdd(&ind_arg6[69+map2idx*225],arg9_l[69]);
    atomicAdd(&ind_arg6[70+map2idx*225],arg9_l[70]);
    atomicAdd(&ind_arg6[71+map2idx*225],arg9_l[71]);
    atomicAdd(&ind_arg6[72+map2idx*225],arg9_l[72]);
    atomicAdd(&ind_arg6[73+map2idx*225],arg9_l[73]);
    atomicAdd(&ind_arg6[74+map2idx*225],arg9_l[74]);
    atomicAdd(&ind_arg6[75+map2idx*225],arg9_l[75]);
    atomicAdd(&ind_arg6[76+map2idx*225],arg9_l[76]);
    atomicAdd(&ind_arg6[77+map2idx*225],arg9_l[77]);
    atomicAdd(&ind_arg6[78+map2idx*225],arg9_l[78]);
    atomicAdd(&ind_arg6[79+map2idx*225],arg9_l[79]);
    atomicAdd(&ind_arg6[80+map2idx*225],arg9_l[80]);
    atomicAdd(&ind_arg6[81+map2idx*225],arg9_l[81]);
    atomicAdd(&ind_arg6[82+map2idx*225],arg9_l[82]);
    atomicAdd(&ind_arg6[83+map2idx*225],arg9_l[83]);
    atomicAdd(&ind_arg6[84+map2idx*225],arg9_l[84]);
    atomicAdd(&ind_arg6[85+map2idx*225],arg9_l[85]);
    atomicAdd(&ind_arg6[86+map2idx*225],arg9_l[86]);
    atomicAdd(&ind_arg6[87+map2idx*225],arg9_l[87]);
    atomicAdd(&ind_arg6[88+map2idx*225],arg9_l[88]);
    atomicAdd(&ind_arg6[89+map2idx*225],arg9_l[89]);
    atomicAdd(&ind_arg6[90+map2idx*225],arg9_l[90]);
    atomicAdd(&ind_arg6[91+map2idx*225],arg9_l[91]);
    atomicAdd(&ind_arg6[92+map2idx*225],arg9_l[92]);
    atomicAdd(&ind_arg6[93+map2idx*225],arg9_l[93]);
    atomicAdd(&ind_arg6[94+map2idx*225],arg9_l[94]);
    atomicAdd(&ind_arg6[95+map2idx*225],arg9_l[95]);
    atomicAdd(&ind_arg6[96+map2idx*225],arg9_l[96]);
    atomicAdd(&ind_arg6[97+map2idx*225],arg9_l[97]);
    atomicAdd(&ind_arg6[98+map2idx*225],arg9_l[98]);
    atomicAdd(&ind_arg6[99+map2idx*225],arg9_l[99]);
    atomicAdd(&ind_arg6[100+map2idx*225],arg9_l[100]);
    atomicAdd(&ind_arg6[101+map2idx*225],arg9_l[101]);
    atomicAdd(&ind_arg6[102+map2idx*225],arg9_l[102]);
    atomicAdd(&ind_arg6[103+map2idx*225],arg9_l[103]);
    atomicAdd(&ind_arg6[104+map2idx*225],arg9_l[104]);
    atomicAdd(&ind_arg6[105+map2idx*225],arg9_l[105]);
    atomicAdd(&ind_arg6[106+map2idx*225],arg9_l[106]);
    atomicAdd(&ind_arg6[107+map2idx*225],arg9_l[107]);
    atomicAdd(&ind_arg6[108+map2idx*225],arg9_l[108]);
    atomicAdd(&ind_arg6[109+map2idx*225],arg9_l[109]);
    atomicAdd(&ind_arg6[110+map2idx*225],arg9_l[110]);
    atomicAdd(&ind_arg6[111+map2idx*225],arg9_l[111]);
    atomicAdd(&ind_arg6[112+map2idx*225],arg9_l[112]);
    atomicAdd(&ind_arg6[113+map2idx*225],arg9_l[113]);
    atomicAdd(&ind_arg6[114+map2idx*225],arg9_l[114]);
    atomicAdd(&ind_arg6[115+map2idx*225],arg9_l[115]);
    atomicAdd(&ind_arg6[116+map2idx*225],arg9_l[116]);
    atomicAdd(&ind_arg6[117+map2idx*225],arg9_l[117]);
    atomicAdd(&ind_arg6[118+map2idx*225],arg9_l[118]);
    atomicAdd(&ind_arg6[119+map2idx*225],arg9_l[119]);
    atomicAdd(&ind_arg6[120+map2idx*225],arg9_l[120]);
    atomicAdd(&ind_arg6[121+map2idx*225],arg9_l[121]);
    atomicAdd(&ind_arg6[122+map2idx*225],arg9_l[122]);
    atomicAdd(&ind_arg6[123+map2idx*225],arg9_l[123]);
    atomicAdd(&ind_arg6[124+map2idx*225],arg9_l[124]);
    atomicAdd(&ind_arg6[125+map2idx*225],arg9_l[125]);
    atomicAdd(&ind_arg6[126+map2idx*225],arg9_l[126]);
    atomicAdd(&ind_arg6[127+map2idx*225],arg9_l[127]);
    atomicAdd(&ind_arg6[128+map2idx*225],arg9_l[128]);
    atomicAdd(&ind_arg6[129+map2idx*225],arg9_l[129]);
    atomicAdd(&ind_arg6[130+map2idx*225],arg9_l[130]);
    atomicAdd(&ind_arg6[131+map2idx*225],arg9_l[131]);
    atomicAdd(&ind_arg6[132+map2idx*225],arg9_l[132]);
    atomicAdd(&ind_arg6[133+map2idx*225],arg9_l[133]);
    atomicAdd(&ind_arg6[134+map2idx*225],arg9_l[134]);
    atomicAdd(&ind_arg6[135+map2idx*225],arg9_l[135]);
    atomicAdd(&ind_arg6[136+map2idx*225],arg9_l[136]);
    atomicAdd(&ind_arg6[137+map2idx*225],arg9_l[137]);
    atomicAdd(&ind_arg6[138+map2idx*225],arg9_l[138]);
    atomicAdd(&ind_arg6[139+map2idx*225],arg9_l[139]);
    atomicAdd(&ind_arg6[140+map2idx*225],arg9_l[140]);
    atomicAdd(&ind_arg6[141+map2idx*225],arg9_l[141]);
    atomicAdd(&ind_arg6[142+map2idx*225],arg9_l[142]);
    atomicAdd(&ind_arg6[143+map2idx*225],arg9_l[143]);
    atomicAdd(&ind_arg6[144+map2idx*225],arg9_l[144]);
    atomicAdd(&ind_arg6[145+map2idx*225],arg9_l[145]);
    atomicAdd(&ind_arg6[146+map2idx*225],arg9_l[146]);
    atomicAdd(&ind_arg6[147+map2idx*225],arg9_l[147]);
    atomicAdd(&ind_arg6[148+map2idx*225],arg9_l[148]);
    atomicAdd(&ind_arg6[149+map2idx*225],arg9_l[149]);
    atomicAdd(&ind_arg6[150+map2idx*225],arg9_l[150]);
    atomicAdd(&ind_arg6[151+map2idx*225],arg9_l[151]);
    atomicAdd(&ind_arg6[152+map2idx*225],arg9_l[152]);
    atomicAdd(&ind_arg6[153+map2idx*225],arg9_l[153]);
    atomicAdd(&ind_arg6[154+map2idx*225],arg9_l[154]);
    atomicAdd(&ind_arg6[155+map2idx*225],arg9_l[155]);
    atomicAdd(&ind_arg6[156+map2idx*225],arg9_l[156]);
    atomicAdd(&ind_arg6[157+map2idx*225],arg9_l[157]);
    atomicAdd(&ind_arg6[158+map2idx*225],arg9_l[158]);
    atomicAdd(&ind_arg6[159+map2idx*225],arg9_l[159]);
    atomicAdd(&ind_arg6[160+map2idx*225],arg9_l[160]);
    atomicAdd(&ind_arg6[161+map2idx*225],arg9_l[161]);
    atomicAdd(&ind_arg6[162+map2idx*225],arg9_l[162]);
    atomicAdd(&ind_arg6[163+map2idx*225],arg9_l[163]);
    atomicAdd(&ind_arg6[164+map2idx*225],arg9_l[164]);
    atomicAdd(&ind_arg6[165+map2idx*225],arg9_l[165]);
    atomicAdd(&ind_arg6[166+map2idx*225],arg9_l[166]);
    atomicAdd(&ind_arg6[167+map2idx*225],arg9_l[167]);
    atomicAdd(&ind_arg6[168+map2idx*225],arg9_l[168]);
    atomicAdd(&ind_arg6[169+map2idx*225],arg9_l[169]);
    atomicAdd(&ind_arg6[170+map2idx*225],arg9_l[170]);
    atomicAdd(&ind_arg6[171+map2idx*225],arg9_l[171]);
    atomicAdd(&ind_arg6[172+map2idx*225],arg9_l[172]);
    atomicAdd(&ind_arg6[173+map2idx*225],arg9_l[173]);
    atomicAdd(&ind_arg6[174+map2idx*225],arg9_l[174]);
    atomicAdd(&ind_arg6[175+map2idx*225],arg9_l[175]);
    atomicAdd(&ind_arg6[176+map2idx*225],arg9_l[176]);
    atomicAdd(&ind_arg6[177+map2idx*225],arg9_l[177]);
    atomicAdd(&ind_arg6[178+map2idx*225],arg9_l[178]);
    atomicAdd(&ind_arg6[179+map2idx*225],arg9_l[179]);
    atomicAdd(&ind_arg6[180+map2idx*225],arg9_l[180]);
    atomicAdd(&ind_arg6[181+map2idx*225],arg9_l[181]);
    atomicAdd(&ind_arg6[182+map2idx*225],arg9_l[182]);
    atomicAdd(&ind_arg6[183+map2idx*225],arg9_l[183]);
    atomicAdd(&ind_arg6[184+map2idx*225],arg9_l[184]);
    atomicAdd(&ind_arg6[185+map2idx*225],arg9_l[185]);
    atomicAdd(&ind_arg6[186+map2idx*225],arg9_l[186]);
    atomicAdd(&ind_arg6[187+map2idx*225],arg9_l[187]);
    atomicAdd(&ind_arg6[188+map2idx*225],arg9_l[188]);
    atomicAdd(&ind_arg6[189+map2idx*225],arg9_l[189]);
    atomicAdd(&ind_arg6[190+map2idx*225],arg9_l[190]);
    atomicAdd(&ind_arg6[191+map2idx*225],arg9_l[191]);
    atomicAdd(&ind_arg6[192+map2idx*225],arg9_l[192]);
    atomicAdd(&ind_arg6[193+map2idx*225],arg9_l[193]);
    atomicAdd(&ind_arg6[194+map2idx*225],arg9_l[194]);
    atomicAdd(&ind_arg6[195+map2idx*225],arg9_l[195]);
    atomicAdd(&ind_arg6[196+map2idx*225],arg9_l[196]);
    atomicAdd(&ind_arg6[197+map2idx*225],arg9_l[197]);
    atomicAdd(&ind_arg6[198+map2idx*225],arg9_l[198]);
    atomicAdd(&ind_arg6[199+map2idx*225],arg9_l[199]);
    atomicAdd(&ind_arg6[200+map2idx*225],arg9_l[200]);
    atomicAdd(&ind_arg6[201+map2idx*225],arg9_l[201]);
    atomicAdd(&ind_arg6[202+map2idx*225],arg9_l[202]);
    atomicAdd(&ind_arg6[203+map2idx*225],arg9_l[203]);
    atomicAdd(&ind_arg6[204+map2idx*225],arg9_l[204]);
    atomicAdd(&ind_arg6[205+map2idx*225],arg9_l[205]);
    atomicAdd(&ind_arg6[206+map2idx*225],arg9_l[206]);
    atomicAdd(&ind_arg6[207+map2idx*225],arg9_l[207]);
    atomicAdd(&ind_arg6[208+map2idx*225],arg9_l[208]);
    atomicAdd(&ind_arg6[209+map2idx*225],arg9_l[209]);
    atomicAdd(&ind_arg6[210+map2idx*225],arg9_l[210]);
    atomicAdd(&ind_arg6[211+map2idx*225],arg9_l[211]);
    atomicAdd(&ind_arg6[212+map2idx*225],arg9_l[212]);
    atomicAdd(&ind_arg6[213+map2idx*225],arg9_l[213]);
    atomicAdd(&ind_arg6[214+map2idx*225],arg9_l[214]);
    atomicAdd(&ind_arg6[215+map2idx*225],arg9_l[215]);
    atomicAdd(&ind_arg6[216+map2idx*225],arg9_l[216]);
    atomicAdd(&ind_arg6[217+map2idx*225],arg9_l[217]);
    atomicAdd(&ind_arg6[218+map2idx*225],arg9_l[218]);
    atomicAdd(&ind_arg6[219+map2idx*225],arg9_l[219]);
    atomicAdd(&ind_arg6[220+map2idx*225],arg9_l[220]);
    atomicAdd(&ind_arg6[221+map2idx*225],arg9_l[221]);
    atomicAdd(&ind_arg6[222+map2idx*225],arg9_l[222]);
    atomicAdd(&ind_arg6[223+map2idx*225],arg9_l[223]);
    atomicAdd(&ind_arg6[224+map2idx*225],arg9_l[224]);
    atomicAdd(&ind_arg6[0+map10idx*225],arg17_l[0]);
    atomicAdd(&ind_arg6[1+map10idx*225],arg17_l[1]);
    atomicAdd(&ind_arg6[2+map10idx*225],arg17_l[2]);
    atomicAdd(&ind_arg6[3+map10idx*225],arg17_l[3]);
    atomicAdd(&ind_arg6[4+map10idx*225],arg17_l[4]);
    atomicAdd(&ind_arg6[5+map10idx*225],arg17_l[5]);
    atomicAdd(&ind_arg6[6+map10idx*225],arg17_l[6]);
    atomicAdd(&ind_arg6[7+map10idx*225],arg17_l[7]);
    atomicAdd(&ind_arg6[8+map10idx*225],arg17_l[8]);
    atomicAdd(&ind_arg6[9+map10idx*225],arg17_l[9]);
    atomicAdd(&ind_arg6[10+map10idx*225],arg17_l[10]);
    atomicAdd(&ind_arg6[11+map10idx*225],arg17_l[11]);
    atomicAdd(&ind_arg6[12+map10idx*225],arg17_l[12]);
    atomicAdd(&ind_arg6[13+map10idx*225],arg17_l[13]);
    atomicAdd(&ind_arg6[14+map10idx*225],arg17_l[14]);
    atomicAdd(&ind_arg6[15+map10idx*225],arg17_l[15]);
    atomicAdd(&ind_arg6[16+map10idx*225],arg17_l[16]);
    atomicAdd(&ind_arg6[17+map10idx*225],arg17_l[17]);
    atomicAdd(&ind_arg6[18+map10idx*225],arg17_l[18]);
    atomicAdd(&ind_arg6[19+map10idx*225],arg17_l[19]);
    atomicAdd(&ind_arg6[20+map10idx*225],arg17_l[20]);
    atomicAdd(&ind_arg6[21+map10idx*225],arg17_l[21]);
    atomicAdd(&ind_arg6[22+map10idx*225],arg17_l[22]);
    atomicAdd(&ind_arg6[23+map10idx*225],arg17_l[23]);
    atomicAdd(&ind_arg6[24+map10idx*225],arg17_l[24]);
    atomicAdd(&ind_arg6[25+map10idx*225],arg17_l[25]);
    atomicAdd(&ind_arg6[26+map10idx*225],arg17_l[26]);
    atomicAdd(&ind_arg6[27+map10idx*225],arg17_l[27]);
    atomicAdd(&ind_arg6[28+map10idx*225],arg17_l[28]);
    atomicAdd(&ind_arg6[29+map10idx*225],arg17_l[29]);
    atomicAdd(&ind_arg6[30+map10idx*225],arg17_l[30]);
    atomicAdd(&ind_arg6[31+map10idx*225],arg17_l[31]);
    atomicAdd(&ind_arg6[32+map10idx*225],arg17_l[32]);
    atomicAdd(&ind_arg6[33+map10idx*225],arg17_l[33]);
    atomicAdd(&ind_arg6[34+map10idx*225],arg17_l[34]);
    atomicAdd(&ind_arg6[35+map10idx*225],arg17_l[35]);
    atomicAdd(&ind_arg6[36+map10idx*225],arg17_l[36]);
    atomicAdd(&ind_arg6[37+map10idx*225],arg17_l[37]);
    atomicAdd(&ind_arg6[38+map10idx*225],arg17_l[38]);
    atomicAdd(&ind_arg6[39+map10idx*225],arg17_l[39]);
    atomicAdd(&ind_arg6[40+map10idx*225],arg17_l[40]);
    atomicAdd(&ind_arg6[41+map10idx*225],arg17_l[41]);
    atomicAdd(&ind_arg6[42+map10idx*225],arg17_l[42]);
    atomicAdd(&ind_arg6[43+map10idx*225],arg17_l[43]);
    atomicAdd(&ind_arg6[44+map10idx*225],arg17_l[44]);
    atomicAdd(&ind_arg6[45+map10idx*225],arg17_l[45]);
    atomicAdd(&ind_arg6[46+map10idx*225],arg17_l[46]);
    atomicAdd(&ind_arg6[47+map10idx*225],arg17_l[47]);
    atomicAdd(&ind_arg6[48+map10idx*225],arg17_l[48]);
    atomicAdd(&ind_arg6[49+map10idx*225],arg17_l[49]);
    atomicAdd(&ind_arg6[50+map10idx*225],arg17_l[50]);
    atomicAdd(&ind_arg6[51+map10idx*225],arg17_l[51]);
    atomicAdd(&ind_arg6[52+map10idx*225],arg17_l[52]);
    atomicAdd(&ind_arg6[53+map10idx*225],arg17_l[53]);
    atomicAdd(&ind_arg6[54+map10idx*225],arg17_l[54]);
    atomicAdd(&ind_arg6[55+map10idx*225],arg17_l[55]);
    atomicAdd(&ind_arg6[56+map10idx*225],arg17_l[56]);
    atomicAdd(&ind_arg6[57+map10idx*225],arg17_l[57]);
    atomicAdd(&ind_arg6[58+map10idx*225],arg17_l[58]);
    atomicAdd(&ind_arg6[59+map10idx*225],arg17_l[59]);
    atomicAdd(&ind_arg6[60+map10idx*225],arg17_l[60]);
    atomicAdd(&ind_arg6[61+map10idx*225],arg17_l[61]);
    atomicAdd(&ind_arg6[62+map10idx*225],arg17_l[62]);
    atomicAdd(&ind_arg6[63+map10idx*225],arg17_l[63]);
    atomicAdd(&ind_arg6[64+map10idx*225],arg17_l[64]);
    atomicAdd(&ind_arg6[65+map10idx*225],arg17_l[65]);
    atomicAdd(&ind_arg6[66+map10idx*225],arg17_l[66]);
    atomicAdd(&ind_arg6[67+map10idx*225],arg17_l[67]);
    atomicAdd(&ind_arg6[68+map10idx*225],arg17_l[68]);
    atomicAdd(&ind_arg6[69+map10idx*225],arg17_l[69]);
    atomicAdd(&ind_arg6[70+map10idx*225],arg17_l[70]);
    atomicAdd(&ind_arg6[71+map10idx*225],arg17_l[71]);
    atomicAdd(&ind_arg6[72+map10idx*225],arg17_l[72]);
    atomicAdd(&ind_arg6[73+map10idx*225],arg17_l[73]);
    atomicAdd(&ind_arg6[74+map10idx*225],arg17_l[74]);
    atomicAdd(&ind_arg6[75+map10idx*225],arg17_l[75]);
    atomicAdd(&ind_arg6[76+map10idx*225],arg17_l[76]);
    atomicAdd(&ind_arg6[77+map10idx*225],arg17_l[77]);
    atomicAdd(&ind_arg6[78+map10idx*225],arg17_l[78]);
    atomicAdd(&ind_arg6[79+map10idx*225],arg17_l[79]);
    atomicAdd(&ind_arg6[80+map10idx*225],arg17_l[80]);
    atomicAdd(&ind_arg6[81+map10idx*225],arg17_l[81]);
    atomicAdd(&ind_arg6[82+map10idx*225],arg17_l[82]);
    atomicAdd(&ind_arg6[83+map10idx*225],arg17_l[83]);
    atomicAdd(&ind_arg6[84+map10idx*225],arg17_l[84]);
    atomicAdd(&ind_arg6[85+map10idx*225],arg17_l[85]);
    atomicAdd(&ind_arg6[86+map10idx*225],arg17_l[86]);
    atomicAdd(&ind_arg6[87+map10idx*225],arg17_l[87]);
    atomicAdd(&ind_arg6[88+map10idx*225],arg17_l[88]);
    atomicAdd(&ind_arg6[89+map10idx*225],arg17_l[89]);
    atomicAdd(&ind_arg6[90+map10idx*225],arg17_l[90]);
    atomicAdd(&ind_arg6[91+map10idx*225],arg17_l[91]);
    atomicAdd(&ind_arg6[92+map10idx*225],arg17_l[92]);
    atomicAdd(&ind_arg6[93+map10idx*225],arg17_l[93]);
    atomicAdd(&ind_arg6[94+map10idx*225],arg17_l[94]);
    atomicAdd(&ind_arg6[95+map10idx*225],arg17_l[95]);
    atomicAdd(&ind_arg6[96+map10idx*225],arg17_l[96]);
    atomicAdd(&ind_arg6[97+map10idx*225],arg17_l[97]);
    atomicAdd(&ind_arg6[98+map10idx*225],arg17_l[98]);
    atomicAdd(&ind_arg6[99+map10idx*225],arg17_l[99]);
    atomicAdd(&ind_arg6[100+map10idx*225],arg17_l[100]);
    atomicAdd(&ind_arg6[101+map10idx*225],arg17_l[101]);
    atomicAdd(&ind_arg6[102+map10idx*225],arg17_l[102]);
    atomicAdd(&ind_arg6[103+map10idx*225],arg17_l[103]);
    atomicAdd(&ind_arg6[104+map10idx*225],arg17_l[104]);
    atomicAdd(&ind_arg6[105+map10idx*225],arg17_l[105]);
    atomicAdd(&ind_arg6[106+map10idx*225],arg17_l[106]);
    atomicAdd(&ind_arg6[107+map10idx*225],arg17_l[107]);
    atomicAdd(&ind_arg6[108+map10idx*225],arg17_l[108]);
    atomicAdd(&ind_arg6[109+map10idx*225],arg17_l[109]);
    atomicAdd(&ind_arg6[110+map10idx*225],arg17_l[110]);
    atomicAdd(&ind_arg6[111+map10idx*225],arg17_l[111]);
    atomicAdd(&ind_arg6[112+map10idx*225],arg17_l[112]);
    atomicAdd(&ind_arg6[113+map10idx*225],arg17_l[113]);
    atomicAdd(&ind_arg6[114+map10idx*225],arg17_l[114]);
    atomicAdd(&ind_arg6[115+map10idx*225],arg17_l[115]);
    atomicAdd(&ind_arg6[116+map10idx*225],arg17_l[116]);
    atomicAdd(&ind_arg6[117+map10idx*225],arg17_l[117]);
    atomicAdd(&ind_arg6[118+map10idx*225],arg17_l[118]);
    atomicAdd(&ind_arg6[119+map10idx*225],arg17_l[119]);
    atomicAdd(&ind_arg6[120+map10idx*225],arg17_l[120]);
    atomicAdd(&ind_arg6[121+map10idx*225],arg17_l[121]);
    atomicAdd(&ind_arg6[122+map10idx*225],arg17_l[122]);
    atomicAdd(&ind_arg6[123+map10idx*225],arg17_l[123]);
    atomicAdd(&ind_arg6[124+map10idx*225],arg17_l[124]);
    atomicAdd(&ind_arg6[125+map10idx*225],arg17_l[125]);
    atomicAdd(&ind_arg6[126+map10idx*225],arg17_l[126]);
    atomicAdd(&ind_arg6[127+map10idx*225],arg17_l[127]);
    atomicAdd(&ind_arg6[128+map10idx*225],arg17_l[128]);
    atomicAdd(&ind_arg6[129+map10idx*225],arg17_l[129]);
    atomicAdd(&ind_arg6[130+map10idx*225],arg17_l[130]);
    atomicAdd(&ind_arg6[131+map10idx*225],arg17_l[131]);
    atomicAdd(&ind_arg6[132+map10idx*225],arg17_l[132]);
    atomicAdd(&ind_arg6[133+map10idx*225],arg17_l[133]);
    atomicAdd(&ind_arg6[134+map10idx*225],arg17_l[134]);
    atomicAdd(&ind_arg6[135+map10idx*225],arg17_l[135]);
    atomicAdd(&ind_arg6[136+map10idx*225],arg17_l[136]);
    atomicAdd(&ind_arg6[137+map10idx*225],arg17_l[137]);
    atomicAdd(&ind_arg6[138+map10idx*225],arg17_l[138]);
    atomicAdd(&ind_arg6[139+map10idx*225],arg17_l[139]);
    atomicAdd(&ind_arg6[140+map10idx*225],arg17_l[140]);
    atomicAdd(&ind_arg6[141+map10idx*225],arg17_l[141]);
    atomicAdd(&ind_arg6[142+map10idx*225],arg17_l[142]);
    atomicAdd(&ind_arg6[143+map10idx*225],arg17_l[143]);
    atomicAdd(&ind_arg6[144+map10idx*225],arg17_l[144]);
    atomicAdd(&ind_arg6[145+map10idx*225],arg17_l[145]);
    atomicAdd(&ind_arg6[146+map10idx*225],arg17_l[146]);
    atomicAdd(&ind_arg6[147+map10idx*225],arg17_l[147]);
    atomicAdd(&ind_arg6[148+map10idx*225],arg17_l[148]);
    atomicAdd(&ind_arg6[149+map10idx*225],arg17_l[149]);
    atomicAdd(&ind_arg6[150+map10idx*225],arg17_l[150]);
    atomicAdd(&ind_arg6[151+map10idx*225],arg17_l[151]);
    atomicAdd(&ind_arg6[152+map10idx*225],arg17_l[152]);
    atomicAdd(&ind_arg6[153+map10idx*225],arg17_l[153]);
    atomicAdd(&ind_arg6[154+map10idx*225],arg17_l[154]);
    atomicAdd(&ind_arg6[155+map10idx*225],arg17_l[155]);
    atomicAdd(&ind_arg6[156+map10idx*225],arg17_l[156]);
    atomicAdd(&ind_arg6[157+map10idx*225],arg17_l[157]);
    atomicAdd(&ind_arg6[158+map10idx*225],arg17_l[158]);
    atomicAdd(&ind_arg6[159+map10idx*225],arg17_l[159]);
    atomicAdd(&ind_arg6[160+map10idx*225],arg17_l[160]);
    atomicAdd(&ind_arg6[161+map10idx*225],arg17_l[161]);
    atomicAdd(&ind_arg6[162+map10idx*225],arg17_l[162]);
    atomicAdd(&ind_arg6[163+map10idx*225],arg17_l[163]);
    atomicAdd(&ind_arg6[164+map10idx*225],arg17_l[164]);
    atomicAdd(&ind_arg6[165+map10idx*225],arg17_l[165]);
    atomicAdd(&ind_arg6[166+map10idx*225],arg17_l[166]);
    atomicAdd(&ind_arg6[167+map10idx*225],arg17_l[167]);
    atomicAdd(&ind_arg6[168+map10idx*225],arg17_l[168]);
    atomicAdd(&ind_arg6[169+map10idx*225],arg17_l[169]);
    atomicAdd(&ind_arg6[170+map10idx*225],arg17_l[170]);
    atomicAdd(&ind_arg6[171+map10idx*225],arg17_l[171]);
    atomicAdd(&ind_arg6[172+map10idx*225],arg17_l[172]);
    atomicAdd(&ind_arg6[173+map10idx*225],arg17_l[173]);
    atomicAdd(&ind_arg6[174+map10idx*225],arg17_l[174]);
    atomicAdd(&ind_arg6[175+map10idx*225],arg17_l[175]);
    atomicAdd(&ind_arg6[176+map10idx*225],arg17_l[176]);
    atomicAdd(&ind_arg6[177+map10idx*225],arg17_l[177]);
    atomicAdd(&ind_arg6[178+map10idx*225],arg17_l[178]);
    atomicAdd(&ind_arg6[179+map10idx*225],arg17_l[179]);
    atomicAdd(&ind_arg6[180+map10idx*225],arg17_l[180]);
    atomicAdd(&ind_arg6[181+map10idx*225],arg17_l[181]);
    atomicAdd(&ind_arg6[182+map10idx*225],arg17_l[182]);
    atomicAdd(&ind_arg6[183+map10idx*225],arg17_l[183]);
    atomicAdd(&ind_arg6[184+map10idx*225],arg17_l[184]);
    atomicAdd(&ind_arg6[185+map10idx*225],arg17_l[185]);
    atomicAdd(&ind_arg6[186+map10idx*225],arg17_l[186]);
    atomicAdd(&ind_arg6[187+map10idx*225],arg17_l[187]);
    atomicAdd(&ind_arg6[188+map10idx*225],arg17_l[188]);
    atomicAdd(&ind_arg6[189+map10idx*225],arg17_l[189]);
    atomicAdd(&ind_arg6[190+map10idx*225],arg17_l[190]);
    atomicAdd(&ind_arg6[191+map10idx*225],arg17_l[191]);
    atomicAdd(&ind_arg6[192+map10idx*225],arg17_l[192]);
    atomicAdd(&ind_arg6[193+map10idx*225],arg17_l[193]);
    atomicAdd(&ind_arg6[194+map10idx*225],arg17_l[194]);
    atomicAdd(&ind_arg6[195+map10idx*225],arg17_l[195]);
    atomicAdd(&ind_arg6[196+map10idx*225],arg17_l[196]);
    atomicAdd(&ind_arg6[197+map10idx*225],arg17_l[197]);
    atomicAdd(&ind_arg6[198+map10idx*225],arg17_l[198]);
    atomicAdd(&ind_arg6[199+map10idx*225],arg17_l[199]);
    atomicAdd(&ind_arg6[200+map10idx*225],arg17_l[200]);
    atomicAdd(&ind_arg6[201+map10idx*225],arg17_l[201]);
    atomicAdd(&ind_arg6[202+map10idx*225],arg17_l[202]);
    atomicAdd(&ind_arg6[203+map10idx*225],arg17_l[203]);
    atomicAdd(&ind_arg6[204+map10idx*225],arg17_l[204]);
    atomicAdd(&ind_arg6[205+map10idx*225],arg17_l[205]);
    atomicAdd(&ind_arg6[206+map10idx*225],arg17_l[206]);
    atomicAdd(&ind_arg6[207+map10idx*225],arg17_l[207]);
    atomicAdd(&ind_arg6[208+map10idx*225],arg17_l[208]);
    atomicAdd(&ind_arg6[209+map10idx*225],arg17_l[209]);
    atomicAdd(&ind_arg6[210+map10idx*225],arg17_l[210]);
    atomicAdd(&ind_arg6[211+map10idx*225],arg17_l[211]);
    atomicAdd(&ind_arg6[212+map10idx*225],arg17_l[212]);
    atomicAdd(&ind_arg6[213+map10idx*225],arg17_l[213]);
    atomicAdd(&ind_arg6[214+map10idx*225],arg17_l[214]);
    atomicAdd(&ind_arg6[215+map10idx*225],arg17_l[215]);
    atomicAdd(&ind_arg6[216+map10idx*225],arg17_l[216]);
    atomicAdd(&ind_arg6[217+map10idx*225],arg17_l[217]);
    atomicAdd(&ind_arg6[218+map10idx*225],arg17_l[218]);
    atomicAdd(&ind_arg6[219+map10idx*225],arg17_l[219]);
    atomicAdd(&ind_arg6[220+map10idx*225],arg17_l[220]);
    atomicAdd(&ind_arg6[221+map10idx*225],arg17_l[221]);
    atomicAdd(&ind_arg6[222+map10idx*225],arg17_l[222]);
    atomicAdd(&ind_arg6[223+map10idx*225],arg17_l[223]);
    atomicAdd(&ind_arg6[224+map10idx*225],arg17_l[224]);
  }
}


//host stub function
void op_par_loop_poisson_mf2_opf(char const *name, op_set set,
  op_arg arg0,
  op_arg arg1,
  op_arg arg2,
  op_arg arg3,
  op_arg arg4,
  op_arg arg5,
  op_arg arg6,
  op_arg arg7,
  op_arg arg8,
  op_arg arg9,
  op_arg arg10,
  op_arg arg11,
  op_arg arg12,
  op_arg arg13,
  op_arg arg14,
  op_arg arg15,
  op_arg arg16,
  op_arg arg17){

  double*arg0h = (double *)arg0.data;
  int nargs = 18;
  op_arg args[18];

  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;
  args[3] = arg3;
  args[4] = arg4;
  args[5] = arg5;
  args[6] = arg6;
  args[7] = arg7;
  args[8] = arg8;
  args[9] = arg9;
  args[10] = arg10;
  args[11] = arg11;
  args[12] = arg12;
  args[13] = arg13;
  args[14] = arg14;
  args[15] = arg15;
  args[16] = arg16;
  args[17] = arg17;

  // initialise timers
  double cpu_t1, cpu_t2, wall_t1, wall_t2;
  op_timing_realloc(22);
  op_timers_core(&cpu_t1, &wall_t1);
  OP_kernels[22].name      = name;
  OP_kernels[22].count    += 1;


  int    ninds   = 7;
  int    inds[18] = {-1,-1,0,1,2,3,4,5,-1,6,0,1,2,3,4,5,-1,6};

  if (OP_diags>2) {
    printf(" kernel routine with indirection: poisson_mf2_opf\n");
  }
  int set_size = op_mpi_halo_exchanges_grouped(set, nargs, args, 2);
  if (set_size > 0) {

    //transfer constants to GPU
    int consts_bytes = 0;
    consts_bytes += ROUND_UP(1*sizeof(double));
    reallocConstArrays(consts_bytes);
    consts_bytes = 0;
    arg0.data   = OP_consts_h + consts_bytes;
    arg0.data_d = OP_consts_d + consts_bytes;
    for ( int d=0; d<1; d++ ){
      ((double *)arg0.data)[d] = arg0h[d];
    }
    consts_bytes += ROUND_UP(1*sizeof(double));
    mvConstArraysToDevice(consts_bytes);

    //set CUDA execution parameters
    #ifdef OP_BLOCK_SIZE_22
      int nthread = OP_BLOCK_SIZE_22;
    #else
      int nthread = OP_block_size;
    #endif

    for ( int round=0; round<2; round++ ){
      if (round==1) {
        op_mpi_wait_all_grouped(nargs, args, 2);
      }
      int start = round==0 ? 0 : set->core_size;
      int end = round==0 ? set->core_size : set->size + set->exec_size;
      if (end-start>0) {
        int nblocks = (end-start-1)/nthread+1;
        op_cuda_poisson_mf2_opf<<<nblocks,nthread>>>(
        (double *)arg2.data_d,
        (double *)arg3.data_d,
        (double *)arg4.data_d,
        (double *)arg5.data_d,
        (double *)arg6.data_d,
        (double *)arg7.data_d,
        (double *)arg9.data_d,
        arg2.map_data_d,
        (double*)arg0.data_d,
        (int*)arg1.data_d,
        (double*)arg8.data_d,
        (double*)arg16.data_d,
        start,end,set->size+set->exec_size);
      }
    }
  }
  op_mpi_set_dirtybit_cuda(nargs, args);
  cutilSafeCall(cudaDeviceSynchronize());
  //update kernel record
  op_timers_core(&cpu_t2, &wall_t2);
  OP_kernels[22].time     += wall_t2 - wall_t1;
}
