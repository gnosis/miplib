
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
    case miplib_SolverBackend::Any:
      return Solver::Backend::Any;
  }
  throw std::logic_error("Unsupported backend.");
}

extern "C" {

char const* get_last_error()
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

}
