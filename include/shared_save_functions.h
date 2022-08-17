#ifndef __INS_SHARED_SAVE_FUNCTIONS_H
#define __INS_SHARED_SAVE_FUNCTIONS_H

#include <vector>
#ifdef INS_MPI
#include "pcgnslib.h"
#else
#include "cgnslib.h"
#endif

void get_cells_order_4(std::vector<double> &x_v, std::vector<double> &y_v,
                       std::vector<cgsize_t> &cells, double *x, double *y,
                       int numCells);

void get_cells_order_3(std::vector<double> &x_v, std::vector<double> &y_v,
                       std::vector<cgsize_t> &cells, double *x, double *y,
                       int numCells);

void get_cells_order_2(std::vector<double> &x_v, std::vector<double> &y_v,
                       std::vector<cgsize_t> &cells, double *x, double *y,
                       int numCells);

void get_cells_order_1(std::vector<double> &x_v, std::vector<double> &y_v,
                       std::vector<cgsize_t> &cells, double *x, double *y,
                       int numCells);

void get_data_vectors_order_4(std::vector<double> &x_v, std::vector<double> &y_v,
                              std::vector<double> &u_v, std::vector<double> &v_v,
                              std::vector<double> &pr_v, std::vector<double> &vort_v,
                              std::vector<double> &s_v, std::vector<double> &o_v,
                              std::vector<cgsize_t> &cells, double *Ux,
                              double *Uy, double *pr, double *vort, double *x,
                              double *y, double *s, int *o, int numCells);

void get_data_vectors_order_3(std::vector<double> &x_v, std::vector<double> &y_v,
                              std::vector<double> &u_v, std::vector<double> &v_v,
                              std::vector<double> &pr_v, std::vector<double> &vort_v,
                              std::vector<double> &s_v, std::vector<double> &o_v,
                              std::vector<cgsize_t> &cells, double *Ux,
                              double *Uy, double *pr, double *vort, double *x,
                              double *y, double *s, int *o, int numCells);

void get_data_vectors_order_2(std::vector<double> &x_v, std::vector<double> &y_v,
                              std::vector<double> &u_v, std::vector<double> &v_v,
                              std::vector<double> &pr_v, std::vector<double> &vort_v,
                              std::vector<double> &s_v, std::vector<double> &o_v,
                              std::vector<cgsize_t> &cells, double *Ux,
                              double *Uy, double *pr, double *vort, double *x,
                              double *y, double *s, int *o, int numCells);

void get_data_vectors_order_1(std::vector<double> &x_v, std::vector<double> &y_v,
                              std::vector<double> &u_v, std::vector<double> &v_v,
                              std::vector<double> &pr_v, std::vector<double> &vort_v,
                              std::vector<double> &s_v, std::vector<double> &o_v,
                              std::vector<cgsize_t> &cells, double *Ux,
                              double *Uy, double *pr, double *vort, double *x,
                              double *y, double *s, int *o, int numCells);

void get_save_data_order_4(std::vector<double> &x_v, std::vector<double> &y_v,
                           std::vector<std::vector<double>> &vals_v,
                           std::vector<cgsize_t> &cells, double *x, double *y,
                           double **vals_data, int numVals, int numCells);

void get_save_data_order_3(std::vector<double> &x_v, std::vector<double> &y_v,
                           std::vector<std::vector<double>> &vals_v,
                           std::vector<cgsize_t> &cells, double *x, double *y,
                           double **vals_data, int numVals, int numCells);

void get_save_data_order_2(std::vector<double> &x_v, std::vector<double> &y_v,
                           std::vector<std::vector<double>> &vals_v,
                           std::vector<cgsize_t> &cells, double *x, double *y,
                           double **vals_data, int numVals, int numCells);

void get_save_data_order_1(std::vector<double> &x_v, std::vector<double> &y_v,
                           std::vector<std::vector<double>> &vals_v,
                           std::vector<cgsize_t> &cells, double *x, double *y,
                           double **vals_data, int numVals, int numCells);
#endif
