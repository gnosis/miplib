#pragma once

#include "solver.hpp"

extern "C" {

enum miplib_SolverBackend {
  Gurobi, Scip, Lpsolve, Any
};

char const* get_last_error();
int miplib_create_solver(miplib::Solver** rp_solver, miplib_SolverBackend backend);
int miplib_destroy_solver(miplib::Solver* p_solver);

}
