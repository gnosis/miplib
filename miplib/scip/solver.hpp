#pragma once

#include <miplib/solver.hpp>

#include <scip/scip.h>

namespace miplib {

struct LazyConstrHandler;

struct ScipSolver : detail::ISolver
{
  ScipSolver();
  virtual ~ScipSolver() noexcept(false);

  std::shared_ptr<detail::IVar> create_var(
    Solver const& solver,
    Var::Type const& type,
    std::optional<double> const& lb,
    std::optional<double> const& ub,
    std::optional<std::string> const& name
  );

  std::shared_ptr<detail::IConstr> create_constr(
    Constr::Type const& type, Expr const& e, std::optional<std::string> const& name
  );

  std::shared_ptr<detail::IIndicatorConstr> create_indicator_constr(
    Constr const& implicant,
    Constr const& implicand,
    std::optional<std::string> const& name
  );

  void set_objective(Solver::Sense const& sense, Expr const& e);

  void add(Constr const& constr);
  void add(IndicatorConstr const& constr);

  void set_lazy_constr_handler(LazyConstrHandler const& constr);

  std::pair<Solver::Result, bool> solve();

  void set_non_convex_policy(Solver::NonConvexPolicy policy);
  void set_int_feasibility_tolerance(double value);
  void set_feasibility_tolerance(double value);

  void set_verbose(bool value);

  SCIP_CONS* as_scip_constr(Constr const& constr);

  bool supports_indicator_constraint(IndicatorConstr const& constr) const;

  bool supports_quadratic_constraints() const { return true; }
  bool supports_quadratic_objective() const { return true; }

  double infinity() const;

  void dump(std::string const& filename) const;

  SCIP* p_env;
  SCIP_SOL* p_sol;
  Var* p_aux_obj_var;
};

}  // namespace miplib
