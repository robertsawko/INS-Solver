//
// auto-generated by op2.py
//

// global constants
extern double gam;
extern double mu;
extern double nu;
extern double bc_mach;
extern double bc_alpha;
extern double bc_p;
extern double bc_u;
extern double bc_v;
extern int FMASK[15];
extern double ic_u;
extern double ic_v;
extern double cubW[46];
extern double cubV[690];
extern double cubVDr[690];
extern double cubVDs[690];
extern double gF0Dr[105];
extern double gF0Ds[105];
extern double gF1Dr[105];
extern double gF1Ds[105];
extern double gF2Dr[105];
extern double gF2Ds[105];
extern double gaussW[7];
extern double gFInterp0[105];
extern double gFInterp1[105];
extern double gFInterp2[105];
extern double gF0DrR[105];
extern double gF0DsR[105];
extern double gF1DrR[105];
extern double gF1DsR[105];
extern double gF2DrR[105];
extern double gF2DsR[105];
extern double gFInterp0R[105];
extern double gFInterp1R[105];
extern double gFInterp2R[105];
extern double lift_drag_vec[5];

// header
#include "op_lib_cpp.h"

// user kernel files
#include "set_ic_seqkernel.cpp"
#include "calc_dt_seqkernel.cpp"
#include "advection_flux_seqkernel.cpp"
#include "advection_faces_seqkernel.cpp"
#include "advection_bc_seqkernel.cpp"
#include "advection_numerical_flux_seqkernel.cpp"
#include "advection_intermediate_vel_seqkernel.cpp"
#include "pressure_bc_seqkernel.cpp"
#include "pressure_rhs_seqkernel.cpp"
#include "pressure_update_vel_seqkernel.cpp"
#include "viscosity_bc_seqkernel.cpp"
#include "viscosity_rhs_seqkernel.cpp"
#include "viscosity_reset_bc_seqkernel.cpp"
#include "lift_drag_seqkernel.cpp"
#include "min_max_seqkernel.cpp"
#include "init_grid_seqkernel.cpp"
#include "init_cubature_grad_seqkernel.cpp"
#include "init_cubature_seqkernel.cpp"
#include "init_cubature_OP_seqkernel.cpp"
#include "gauss_reverse_seqkernel.cpp"
#include "init_gauss_seqkernel.cpp"
#include "gauss_tau_seqkernel.cpp"
#include "gauss_tau_bc_seqkernel.cpp"
#include "init_gauss_grad_seqkernel.cpp"
#include "init_gauss_grad2_seqkernel.cpp"
#include "init_gauss_grad_neighbour_seqkernel.cpp"
#include "gauss_grad_faces_seqkernel.cpp"
#include "gauss_op_seqkernel.cpp"
#include "gauss_gfi_faces_seqkernel.cpp"
#include "div_seqkernel.cpp"
#include "curl_seqkernel.cpp"
#include "grad_seqkernel.cpp"
#include "cub_grad_seqkernel.cpp"
#include "cub_div_seqkernel.cpp"
#include "poisson_rhs_faces_seqkernel.cpp"
#include "poisson_rhs_bc_seqkernel.cpp"
#include "poisson_rhs_flux_seqkernel.cpp"
#include "poisson_rhs_J_seqkernel.cpp"
#include "poisson_rhs_qbc_seqkernel.cpp"
#include "poisson_rhs_qflux_seqkernel.cpp"
