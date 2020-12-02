#include "solver.hpp"

#ifdef WITH_GUROBI
#  include "gurobi/solver.hpp"
#endif

#ifdef WITH_SCIP
#  include "scip/solver.hpp"
#endif

#ifdef WITH_LPSOLVE
#  include "lpsolve/solver.hpp"
#endif

namespace miplib {

Solver::Solver(Backend backend): m_backend(backend)
{
  if (backend == Solver::Backend::Gurobi)
  {
    #ifdef WITH_GUROBI
    p_impl = std::make_shared<GurobiSolver>();
    #else
    throw std::logic_error("Request for Gurobi backend but it was not compiled.");
    #endif
  }
  else if (backend == Solver::Backend::Scip)
  {
    #ifdef WITH_SCIP
    p_impl = std::make_shared<ScipSolver>();
    #else
    throw std::logic_error("Request for SCIP backend but it was not compiled.");
    #endif
  }
  else if (backend == Solver::Backend::Lpsolve)
  {
    #ifdef WITH_LPSOLVE
    p_impl = std::make_shared<LpsolveSolver>();
    #else
    throw std::logic_error("Request for Lpsolve backend but it was not compiled.");
    #endif
  }
  else if (backend == Solver::Backend::Any)
  {
    #if defined(WITH_GUROBI)
    p_impl = std::make_shared<GurobiSolver>();
    #elif defined(WITH_SCIP)
    p_impl = std::make_shared<ScipSolver>();
    #elif defined(WITH_LPSOLVE)
    p_impl = std::make_shared<LpsolveSolver>();
    #else
    throw std::logic_error("No MIP backends were compiled.");
    #endif
  }
}

void Solver::set_objective(Sense const& sense, Expr const& e)
{
  p_impl->set_objective(sense, e);
}

void Solver::add(Constr const& constr)
{
  p_impl->add(constr);
}

void Solver::add(IndicatorConstr const& constr)
{
  p_impl->add(constr);
}

Solver::Result Solver::solve()
{
  return p_impl->solve();
}

Solver::Result Solver::maximize(Expr const& e)
{
  set_objective(Sense::Maximize, e);
  return solve();
}

Solver::Result Solver::minimize(Expr const& e)
{
  set_objective(Sense::Minimize, e);
  return solve();
}

void Solver::set_non_convex_policy(NonConvexPolicy policy)
{
  p_impl->set_non_convex_policy(policy);
}

void Solver::set_verbose(bool value)
{
  p_impl->set_verbose(value);
}

bool Solver::supports_indicator_constraints() const
{ 
  return p_impl->supports_indicator_constraints(); 
}

bool Solver::supports_quadratic_constraints() const
{
  return p_impl->supports_quadratic_constraints();
}

bool Solver::supports_quadratic_objective() const
{
  return p_impl->supports_quadratic_objective();
}

}  // namespace miplib
