#pragma once

#include "solver.hpp"
#include "var.hpp"

extern "C" {

enum miplib_SolverBackend {
  Gurobi = 0, Scip = 1, Lpsolve = 2, Any = 3
};

enum class miplib_VarType { 
  Continuous = 0, Binary = 1, Integer = 2
};

char const* miplib_get_last_error();
int miplib_create_solver(miplib::Solver** rp_solver, miplib_SolverBackend backend);
int miplib_destroy_solver(miplib::Solver* p_solver);
int miplib_shallow_copy_solver(miplib::Solver** rp_solver, miplib::Solver* p_solver);

int miplib_create_var(miplib::Var** rp_var, miplib::Solver* p_solver, miplib_VarType type);
int miplib_destroy_var(miplib::Var* p_var);
int miplib_shallow_copy_var(miplib::Var** rp_var, miplib::Var* p_var);

}
