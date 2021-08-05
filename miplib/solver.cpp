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

Solver::Solver(Backend backend): m_backend(backend), m_constraint_autoscale(false)
{
  switch (backend)
  {
    case  Solver::Backend::Gurobi:
      #ifdef WITH_GUROBI
      p_impl = std::make_shared<GurobiSolver>();
      return;
      #else
      throw std::logic_error("Request for Gurobi backend but it was not compiled.");
      #endif
    case Solver::Backend::Scip:
      #ifdef WITH_SCIP
      p_impl = std::make_shared<ScipSolver>();
      return;
      #else
      throw std::logic_error("Request for SCIP backend but it was not compiled.");
      #endif
    case Solver::Backend::Lpsolve:
      #ifdef WITH_LPSOLVE
      p_impl = std::make_shared<LpsolveSolver>();
      return;
      #else
      throw std::logic_error("Request for Lpsolve backend but it was not compiled.");
      #endif
    case Solver::Backend::BestAtCompileTime:
      #if defined(WITH_GUROBI)
      p_impl = std::make_shared<GurobiSolver>();
      return;
      #elif defined(WITH_SCIP)
      p_impl = std::make_shared<ScipSolver>();
      return;
      #elif defined(WITH_LPSOLVE)
      p_impl = std::make_shared<LpsolveSolver>();
      return;
      #else
      throw std::logic_error("No MIP backends were compiled.");
      #endif
    case Solver::Backend::BestAtRunTime:
      #if defined(WITH_GUROBI)
      if (backend_is_available(Solver::Backend::Gurobi))
      {
        p_impl = std::make_shared<GurobiSolver>();
        return;  
      }
      #endif
      #if defined(WITH_SCIP)
      if (backend_is_available(Solver::Backend::Scip))
      {
        p_impl = std::make_shared<ScipSolver>();
        return;  
      }
      #endif
      #if defined(WITH_LPSOLVE)
      if (backend_is_available(Solver::Backend::Lpsolve))
      {
        p_impl = std::make_shared<LpsolveSolver>();
        return;  
      }
      #endif
      throw std::logic_error("No MIP backends are available.");
  }
}

void Solver::set_objective(Sense const& sense, Expr const& e)
{
  p_impl->set_objective(sense, e);
}

void Solver::add(Constr const& constr, bool scale)
{
  if (constr.must_be_violated())
    throw std::logic_error("Attempt to create a constraint that is trivially unsat.");

  if (scale or m_constraint_autoscale)
    p_impl->add(constr.scale());
  else
    p_impl->add(constr);
}

void Solver::add(IndicatorConstr const& constr, bool scale)
{
  if (
    p_impl->m_indicator_constraint_policy == 
      Solver::IndicatorConstraintPolicy::Reformulate or
    (
      !supports_indicator_constraint(constr) and
      p_impl->m_indicator_constraint_policy == 
      Solver::IndicatorConstraintPolicy::ReformulateIfUnsupported
    ) or
    scale
  )
  {
    for (auto const& c: constr.reformulation())
      add(c, scale);
  }
  else
    p_impl->add(constr);
}

void Solver::remove(Constr const& constr)
{
  p_impl->remove(constr);
}

void Solver::add_lazy_constr_handler(LazyConstrHandler const& constr_handler, bool at_integral_only)
{
  p_impl->add_lazy_constr_handler(constr_handler, at_integral_only);
}

std::pair<Solver::Result, bool> Solver::solve()
{
  return p_impl->solve();
}

std::pair<Solver::Result, bool> Solver::maximize(Expr const& e)
{
  set_objective(Sense::Maximize, e);
  return solve();
}

std::pair<Solver::Result, bool> Solver::minimize(Expr const& e)
{
  set_objective(Sense::Minimize, e);
  return solve();
}

void Solver::set_non_convex_policy(NonConvexPolicy policy)
{
  p_impl->set_non_convex_policy(policy);
}

void Solver::set_indicator_constraint_policy(IndicatorConstraintPolicy policy)
{
  p_impl->set_indicator_constraint_policy(policy);
}

void Solver::set_verbose(bool value)
{
  p_impl->set_verbose(value);
}

void Solver::set_constraint_autoscale(bool autoscale)
{
  m_constraint_autoscale = autoscale;
}

void Solver::set_feasibility_tolerance(double value)
{
  p_impl->set_feasibility_tolerance(value);
}

void Solver::set_int_feasibility_tolerance(double value)
{
  p_impl->set_int_feasibility_tolerance(value);
}

void Solver::set_epsilon(double value)
{
  p_impl->set_epsilon(value);
}

double Solver::get_int_feasibility_tolerance() const
{
  return p_impl->get_int_feasibility_tolerance();
}

double Solver::get_feasibility_tolerance() const
{
  return p_impl->get_feasibility_tolerance();
}

bool Solver::supports_indicator_constraint(IndicatorConstr const& constr) const
{
  return p_impl->supports_indicator_constraint(constr); 
}

bool Solver::supports_quadratic_constraints() const
{
  return p_impl->supports_quadratic_constraints();
}

bool Solver::supports_quadratic_objective() const
{
  return p_impl->supports_quadratic_objective();
}

double Solver::infinity() const
{
  return p_impl->infinity();
}

void Solver::set_time_limit(double secs)
{
  p_impl->set_time_limit(secs);
}

bool Solver::backend_is_compiled(Backend const& backend)
{
  switch (backend)
  {
    case  Solver::Backend::Gurobi:
      #ifdef WITH_GUROBI
      return true;
      #else
      return false;
      #endif
    case  Solver::Backend::Scip:
      #ifdef WITH_SCIP
      return true;
      #else
      return false;
      #endif
    case  Solver::Backend::Lpsolve:
      #ifdef WITH_LPSOLVE
      return true;
      #else
      return false;
      #endif
    default:
      return false;
  }
}

bool Solver::backend_is_available(Backend const& backend)
{
  if (!backend_is_compiled(backend))
    return false;
  switch (backend)
  {
    case  Solver::Backend::Gurobi:
      #ifdef WITH_GUROBI
      return GurobiSolver::is_available();
      #else
      return false;
      #endif
    case  Solver::Backend::Scip:
      #ifdef WITH_SCIP
      return ScipSolver::is_available();
      #else
      return false;
      #endif
    case  Solver::Backend::Lpsolve:
      #ifdef WITH_LPSOLVE
      return LpsolveSolver::is_available();
      #else
      return false;
      #endif
    default:
      return false;
  }
}

void Solver::dump(std::string const& filename) const
{
  p_impl->dump(filename);
}

void Solver::set_warm_start(PartialSolution const& partial_solution)
{
  p_impl->set_warm_start(partial_solution);
}

std::map<Solver::Backend, std::string> Solver::backend_info()
{
  std::map<Backend, std::string> r;
  #ifdef WITH_GUROBI
  if (backend_is_compiled(Solver::Backend::Gurobi))
    r[Solver::Backend::Gurobi] = GurobiSolver::backend_info();
  #endif
  #ifdef WITH_SCIP
  if (backend_is_compiled(Solver::Backend::Scip))
    r[Solver::Backend::Scip] = ScipSolver::backend_info();
  #endif
  #ifdef WITH_LPSOLVE
  if (backend_is_compiled(Solver::Backend::Lpsolve))
    r[Solver::Backend::Lpsolve] = LpsolveSolver::backend_info();
  #endif
  return r;
}

namespace detail {
void ISolver::set_indicator_constraint_policy(Solver::IndicatorConstraintPolicy policy)
{
  m_indicator_constraint_policy = policy;
}

}

std::ostream& operator<<(std::ostream& os, Solver::Backend const& solver_backend)
{
  switch (solver_backend)
  {
    case Solver::Backend::Scip:
      os << "Scip";
      break;
    case Solver::Backend::Gurobi:
      os << "Gurobi";
      break;
    case Solver::Backend::Lpsolve:
      os << "Lpsolve";
      break;
    case Solver::Backend::BestAtCompileTime:
      os << "BestAtCompileTime";
      break;
    case Solver::Backend::BestAtRunTime:
      os << "BestAtRunTime";
      break;
  }
  return os;
}

}  // namespace miplib
