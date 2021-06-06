#ifndef _MAIN_HPP_
#define _MAIN_HPP_

#include <iostream>
#include <boost/array.hpp>
#include <fstream>
#include <boost/numeric/odeint.hpp>
#include <vector>
#include <Eigen/Dense>
#include <random>
#include <cmath>
#include <chrono>
#include <omp.h>
#define N_SPECIES 3 
#define N 10000 // # of samples to sample over
#define N_DIM 6 // dim of PSO hypercube
#define N_PARTICLES 5 
#define SUBSET_SIZE 3

/* namespaces for ease of use */
using namespace std;
using namespace Eigen;
using namespace boost::numeric::odeint;

/* typedefs for boost ODE-ints*/
typedef boost::numeric::ublas::vector< double > vector_type;
typedef boost::numeric::ublas::matrix< double > matrix_type;
typedef boost::array< double , N_SPECIES > state_N_type;
typedef boost::array< double , 6 > state_6_type;
typedef runge_kutta_cash_karp54< state_N_type > errorRK_stepper_N_type;
typedef runge_kutta_cash_karp54< state_6_type > errorRK_stepper_6_type;
typedef controlled_runge_kutta< errorRK_stepper_N_type > controlledRK_stepper_N_type;
typedef controlled_runge_kutta< errorRK_stepper_6_type > controlledRK_stepper_6_type;

/* Collect data functions - in main for ease of access - @TODO clean up in other files! */
void sample_const( const state_N_type &c , const double t);
void sample_adapt( const state_N_type &c , const double t);
void sample_adapt_linear( const state_N_type &c , const double t);

/* Note: @TODO change global vars back into local vars later */
/* model global diff eq. constants */
const double extern ke, kme, kf, kmf, kd, kmd, ka2, ka3, C1T, C2T, C3T;
/* Bill's K */
const double extern k1, k2, k3, k4, k5;
/* global time conditions */
const double extern t0, tf, dt, tn;
/* Normal Dist Vars */
const double extern mu_x, mu_y, mu_z; // true means for MVN(theta)
const double extern var_x, var_y, var_z; // true variances for MVN(theta);
const double extern rho_xy, rho_xz, rho_yz; // true correlations for MVN
const double extern sigma_x, sigma_y, sigma_z;
#endif