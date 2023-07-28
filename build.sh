#!/bin/bash

set -e

rm -rf build
rm -rf gen2d
rm -rf gen3d

mkdir gen2d
cd gen2d

mkdir -p kernels
mkdir -p solvers/2d/ls_utils
mkdir -p solvers/3d/ls_utils
mkdir -p matrices/2d
mkdir -p matrices/3d/custom_kernels
mkdir -p linear_solvers/amgx_amg
mkdir -p linear_solvers/hypre_amg
mkdir -p linear_solvers/petsc_amg
mkdir -p linear_solvers/petsc_utils
mkdir -p linear_solvers/petsc_block_jacobi
mkdir -p linear_solvers/petsc_jacobi/custom_kernels
mkdir -p linear_solvers/pmultigrid/custom_kernels
mkdir -p linear_solvers/petsc_pmultigrid
mkdir -p linear_solvers/petsc_inv_mass
mkdir -p linear_solvers/initial_guess_extrapolation

cd ..

mkdir gen3d
cd gen3d

mkdir -p kernels
mkdir -p solvers/2d/ls_utils
mkdir -p solvers/3d/ls_utils
mkdir -p matrices/2d
mkdir -p matrices/3d/custom_kernels
mkdir -p linear_solvers/amgx_amg
mkdir -p linear_solvers/petsc_amg
mkdir -p linear_solvers/hypre_amg
mkdir -p linear_solvers/petsc_utils
mkdir -p linear_solvers/petsc_block_jacobi
mkdir -p linear_solvers/petsc_jacobi/custom_kernels
mkdir -p linear_solvers/pmultigrid/custom_kernels
mkdir -p linear_solvers/petsc_pmultigrid
mkdir -p linear_solvers/petsc_inv_mass
mkdir -p linear_solvers/initial_guess_extrapolation

cd ..

ORDER=3
SOA=1

python3 preprocessor.py 2 $ORDER

python3 preprocessor.py 3 $ORDER

if [ $SOA = 1 ]; then
  export OP_AUTO_SOA=1
fi

cd gen2d

# python3 $OP2_TRANSLATOR ins.cpp \
#         ins_data.cpp solver.cpp poisson/petsc/poisson.cpp \
#         ls/ls.cpp utils.cpp utils.cu ls/ls_reinit.cpp ls/ls_reinit.cu \
#         ls/ls_reinit_mpi.cpp ls/ls_reinit_mpi.cu ls/ls_reinit_mpi_naive.cpp \
#         ls/ls_reinit_mpi_naive.cu poisson/matrix/poisson_mat.cpp \
#         poisson/p_multigrid/p_multigrid.cpp kernels/

# sed -i '10i extern double reynolds;' openmp/ins_kernels.cpp

python3 $OP2_TRANSLATOR ins2d.cpp \
        solvers/2d/advection_solver.cpp \
        solvers/2d/advection_solver_over_int.cpp \
        solvers/2d/ls_solver.cpp \
        solvers/2d/mp_ins_solver_over_int.cpp \
        solvers/2d/ins_solver_base.cpp \
        solvers/2d/ins_solver.cpp \
        solvers/2d/mp_ins_solver.cpp \
        solvers/2d/ins_solver_over_int.cpp \
        solvers/2d/ce_solver_over_int.cpp \
        kernels/

sed -i "4i #include \"dg_compiler_defs.h\"" cuda/ins2d_kernels.cu
sed -i "4i #include \"dg_compiler_defs.h\"" openmp/ins2d_kernels.cpp
sed -i "4i #include \"dg_compiler_defs.h\"" seq/ins2d_seqkernels.cpp
sed -i "6i #include \"dg_global_constants/dg_mat_constants_2d.h\"" cuda/ins2d_kernels.cu
sed -i "6i #include \"dg_global_constants/dg_mat_constants_2d.h\"" openmp/ins2d_kernels.cpp
sed -i "6i #include \"dg_global_constants/dg_mat_constants_2d.h\"" seq/ins2d_seqkernels.cpp
sed -i "5i #include \"cblas.h\"" openmp/ins2d_kernels.cpp
sed -i "5i #include \"cblas.h\"" seq/ins2d_seqkernels.cpp

cd ..

cd gen3d

python3 $OP2_TRANSLATOR ins3d.cpp \
        solvers/3d/advection_solver.cpp \
        solvers/3d/ins_solver_base.cpp \
        solvers/3d/ins_solver.cpp \
        solvers/3d/ls_solver.cpp \
        solvers/3d/mp_ins_solver.cpp \
        kernels/

sed -i "4i #include \"dg_compiler_defs.h\"" cuda/ins3d_kernels.cu
sed -i "4i #include \"dg_compiler_defs.h\"" openmp/ins3d_kernels.cpp
sed -i "4i #include \"dg_compiler_defs.h\"" seq/ins3d_seqkernels.cpp
sed -i "5i #include \"liquid_whistle_consts.h\"" cuda/ins3d_kernels.cu
sed -i "5i #include \"liquid_whistle_consts.h\"" openmp/ins3d_kernels.cpp
sed -i "5i #include \"liquid_whistle_consts.h\"" seq/ins3d_seqkernels.cpp
sed -i "6i #include \"dg_global_constants/dg_mat_constants_3d.h\"" cuda/ins3d_kernels.cu
sed -i "6i #include \"dg_global_constants/dg_mat_constants_3d.h\"" openmp/ins3d_kernels.cpp
sed -i "6i #include \"dg_global_constants/dg_mat_constants_3d.h\"" seq/ins3d_seqkernels.cpp
sed -i "7i #include \"cblas.h\"" openmp/ins3d_kernels.cpp
sed -i "7i #include \"cblas.h\"" seq/ins3d_seqkernels.cpp

cd ..

mkdir build

cd build

#-DAMGX_DIR=/home/u1717021/Code/PhD/AMGX-install \
#-DHYPRE_DIR=/home/u1717021/Code/PhD/hypre-install \

cmake .. \
  -DOP2_DIR=/home/u1717021/Code/PhD/OP2-My-Fork/op2 \
  -DOPENBLAS_DIR=/opt/OpenBLAS \
  -DPETSC_DIR=/home/u1717021/Code/PhD/petsc/arch-linux-c-debug \
  -DPART_LIB_NAME=PARMETIS \
  -DPARMETIS_DIR=/home/u1717021/Code/PhD/ParMetis_Libs \
  -DOP2DGTOOLKIT_DIR=/home/u1717021/Code/PhD/OP2-DG-Toolkit/build \
  -DHDF5_DIR=/usr/local/module-software/hdf5-1.12.0-parallel \
  -DARMA_DIR=/home/u1717021/Code/PhD/armadillo-10.5.3/build \
  -DINIPP_DIR=/home/u1717021/Code/PhD/inipp/inipp \
  -DHYPRE_DIR=/home/u1717021/Code/PhD/hypre-cpu-sp-install \
  -DORDER=$ORDER \
  -DSOA=$SOA \
  -DBUILD_SN=ON \
  -DBUILD_CPU=OFF \
  -DBUILD_MPI=ON \
  -DBUILD_GPU=ON

make -j 16
make
