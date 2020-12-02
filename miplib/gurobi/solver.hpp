#pragma once

#include <miplib/solver.hpp>

#include <gurobi_c++.h>

namespace miplib {

struct GurobiSolver : detail::ISolver
{
  GurobiSolver();
  virtual ~GurobiSolver() {}

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
    std::optional<std::string> const& name);

  void set_objective(Solver::Sense const& sense, Expr const& e);

  void add(Constr const& constr);
  void add(IndicatorConstr const& constr);

  Solver::Result solve();

  void set_non_convex_policy(Solver::NonConvexPolicy policy);

  void set_verbose(bool value);

  GRBLinExpr as_grb_lin_expr(Expr const& e);
  GRBQuadExpr as_grb_quad_expr(Expr const& e);

  void set_pending_update() const;
  void update_if_pending() const;

  bool supports_indicator_constraints() const { return true; }
  bool supports_quadratic_constraints() const { return true; }
  bool supports_quadratic_objective() const { return true; }

  GRBEnv env;
  mutable GRBModel model;
  mutable bool pending_update;
};

}  // namespace miplib
