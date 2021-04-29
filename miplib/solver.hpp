#pragma once

#include <memory>

#include "var.hpp"
#include "constr.hpp"
#include "lazy.hpp"

namespace miplib {

struct Solver;

namespace detail {
struct ISolver;
}

struct Solver
{
  enum class Backend { Gurobi, Scip, Lpsolve, Any };
  enum class NonConvexPolicy { Error, Linearize, Branch };
  enum class IndicatorConstraintPolicy { PassThrough, Reformulate, ReformulateIfUnsupported };
  enum class Sense { Maximize, Minimize };
  enum class Result {
    Optimal,
    Infeasible,
    InfeasibleOrUnbounded,
    Unbounded,
    Interrupted,
    Error,
    Other
  };

  Solver(Backend backend);
  Backend const& backend() const
  {
    return m_backend;
  }

  void set_objective(Sense const& sense, Expr const& e);
  void add(Constr const& constr, bool scale = false);
  // note: if scale=true then the constraint will be first
  // reformulated to a linear expression and then scaled.
  void add(IndicatorConstr const& constr, bool scale = false);
  void set_lazy_constr_handler(LazyConstrHandler const& constr_handler);

  void set_non_convex_policy(NonConvexPolicy policy);
  void set_indicator_constraint_policy(IndicatorConstraintPolicy policy);
  void set_constraint_autoscale(bool autoscale);

  // returns Result and if there is a solution.
  std::pair<Result, bool> solve();

  // shortcut for set_objective and solve;
  std::pair<Result, bool> maximize(Expr const& e);
  std::pair<Result, bool> minimize(Expr const& e);

  void set_verbose(bool value);

  bool supports_indicator_constraint(IndicatorConstr const& constr) const;

  bool supports_quadratic_constraints() const;
  bool supports_quadratic_objective() const;

  static bool supports_backend(Backend const&);

  double infinity() const;

  void dump(std::string const& filename) const;

  private:
  std::shared_ptr<detail::ISolver> p_impl;
  const Backend m_backend;
  bool m_constraint_autoscale;
  friend struct Var;
  friend struct Constr;
  friend struct IndicatorConstr;
  friend struct GurobiVar;
  friend struct ScipVar;
  friend struct GurobiLinearConstr;
  friend struct GurobiQuadConstr;
  friend struct GurobiIndicatorConstr;
  friend struct ScipConstr;
  friend struct ScipIndicatorConstr;
  friend struct LpsolveVar;
  friend struct ScipCurrentStateHandle;
};

namespace detail {

struct ISolver
{
  virtual std::shared_ptr<detail::IVar> create_var(
    Solver const& solver,
    Var::Type const& type,
    std::optional<double> const& lb,
    std::optional<double> const& ub,
    std::optional<std::string> const& name
  ) = 0;

  virtual std::shared_ptr<detail::IConstr> create_constr(
    Constr::Type const& type, Expr const& e, std::optional<std::string> const& name
  ) = 0;

  virtual std::shared_ptr<detail::IIndicatorConstr> create_indicator_constr(
    Constr const& implicant,
    Constr const& implicand,
    std::optional<std::string> const& name
  ) = 0;

  virtual void set_objective(Solver::Sense const& sense, Expr const& e) = 0;
  virtual void add(Constr const& constr) = 0;
  virtual void add(IndicatorConstr const& constr) = 0;

  virtual void set_lazy_constr_handler(LazyConstrHandler const& constr) = 0;
  virtual std::pair<Solver::Result, bool> solve() = 0;
  virtual void set_non_convex_policy(Solver::NonConvexPolicy policy) = 0;
  virtual void set_indicator_constraint_policy(Solver::IndicatorConstraintPolicy policy);

  virtual void set_verbose(bool value) = 0;

  virtual bool supports_indicator_constraint(IndicatorConstr const& constr) const = 0;

  virtual bool supports_quadratic_constraints() const = 0;
  virtual bool supports_quadratic_objective() const = 0;

  virtual double infinity() const = 0;

  virtual void dump(std::string const& filename) const = 0;

  Solver::IndicatorConstraintPolicy m_indicator_constraint_policy = 
    Solver::IndicatorConstraintPolicy::ReformulateIfUnsupported;
};

}  // namespace detail

}  // namespace miplib
