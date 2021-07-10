
#include "capi.hpp"

using namespace miplib;

// UTILITY

static std::string last_error;

void store_error(std::string const& err)
{
  last_error = err;
}

template<class Func>
int execute(Func const& f)
{
  try {
    f();
  }
  catch (std::exception const& e) {
    store_error(e.what());
    return 1;
  }
  return 0;
}

#define BEGIN auto const& f = [&]() {
#define END }; return execute(f);

Solver::Backend convert_backend(miplib_SolverBackend backend)
{
  switch (backend)
  {
    case miplib_SolverBackend::Gurobi:
      return Solver::Backend::Gurobi;
    case miplib_SolverBackend::Scip:
      return Solver::Backend::Scip;
    case miplib_SolverBackend::Lpsolve:
      return Solver::Backend::Lpsolve;
    case miplib_SolverBackend::BestAtCompileTime:
      return Solver::Backend::BestAtCompileTime;
    case miplib_SolverBackend::BestAtRunTime:
      return Solver::Backend::BestAtRunTime;
  }
  throw std::logic_error("Unsupported backend.");
}

Var::Type convert_var_type(miplib_VarType type)
{
  switch (type)
  {
    case miplib_VarType::Continuous:
      return Var::Type::Continuous;
    case miplib_VarType::Binary:
      return Var::Type::Binary;
    case miplib_VarType::Integer:
      return Var::Type::Integer;
  }
  throw std::logic_error("Unsupported var type.");
}

extern "C" {

char const* miplib_get_last_error()
{
  return last_error.c_str();
}

int miplib_create_solver(Solver** rp_solver, miplib_SolverBackend backend)
{
  BEGIN
  *rp_solver = new Solver(convert_backend(backend));
  END
}

int miplib_destroy_solver(Solver* p_solver)
{
  BEGIN
  delete p_solver;
  END
}

int miplib_shallow_copy_solver(miplib::Solver** rp_solver, miplib::Solver* p_solver)
{
  BEGIN
  *rp_solver = new Solver(*p_solver);
  END
}

int miplib_create_var(miplib::Var** rp_var, miplib::Solver* p_solver, miplib_VarType type)
{
  BEGIN
  *rp_var = new miplib::Var(*p_solver, convert_var_type(type));
  END
}

int miplib_destroy_var(miplib::Var* p_var)
{
  BEGIN
  delete p_var;
  END
}

int miplib_shallow_copy_var(miplib::Var** rp_var, miplib::Var* p_var)
{
  BEGIN
  *rp_var = new Var(*p_var);
  END
}

}
