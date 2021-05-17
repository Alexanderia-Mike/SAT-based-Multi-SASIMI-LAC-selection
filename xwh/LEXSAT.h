#ifndef _LEXSAT_H_
#define _LEXSAT_H_
#include <sat/bsat/satSolver.h>
#include <sat/cnf/cnf.h>
#include <misc/util/abc_global.h>
#include <base/io/ioAbc.h>
#include <iostream>
#include <random>
#include <aig/ioa/ioa.h>
#include <sat/bsat/satStore.h>
using namespace std;
extern "C" {
Cnf_Dat_t * Cnf_DeriveSimple_xwh( Aig_Man_t * p, int nOutputs );
int sat_solver_solve_lexsat_xwh( sat_solver* s, int * pLits, int nLits );
int sat_solver_solve_maxlexsat_xwh( sat_solver* s, int * pLits, int nLits );
}
#endif