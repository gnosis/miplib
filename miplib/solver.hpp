#pragma once

#include <memory>
#include <map>

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
  enum class Backend { Gurobi, Scip, Lpsolve, BestAtCompileTime, BestAtRunTime };
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

  Solver(Backend backend, bool verbose=true);

  Backend const& backend() const
  {
    return m_backend;
  }

  void set_objective(Sense const& sense, Expr const& e);
  double get_objective_value() const;
  Solver::Sense get_objective_sense() const;

  void add(Constr const& constr, bool scale = false);
  // note: if scale=true then the constraint will be first
  // reformulated to a linear expression and then scaled.
  void add(IndicatorConstr const& constr, bool scale = false);

  void remove(Constr const& constr);

  void add_lazy_constr_handler(LazyConstrHandler const& constr_handler, bool at_integral_only);

  void set_non_convex_policy(NonConvexPolicy policy);
  void set_indicator_constraint_policy(IndicatorConstraintPolicy policy);
  void set_constraint_autoscale(bool autoscale);

  void set_int_feasibility_tolerance(double value);
  void set_feasibility_tolerance(double value);
  void set_epsilon(double value);
  void set_nr_threads(std::size_t);

  double get_int_feasibility_tolerance() const;
  double get_feasibility_tolerance() const;
  double get_epsilon() const;

  // returns Result and if there is a solution.
  std::pair<Result, bool> solve();

  // shortcut for set_objective and solve;
  std::pair<Result, bool> maximize(Expr const& e);
  std::pair<Result, bool> minimize(Expr const& e);

  bool supports_indicator_constraint(IndicatorConstr const& constr) const;

  bool supports_quadratic_constraints() const;
  bool supports_quadratic_objective() const;

  // if backend was found at compile time
  static bool backend_is_compiled(Backend const&);

  // if backend is available for use at runtime time
  // (a backend that is compiled may not be available
  // due e.g. to expired license file)
  static bool backend_is_available(Backend const&);

  double infinity() const;

  void dump(std::string const& filename) const;

  void set_time_limit(double secs);

  // Used to build initial feasible solution.
  void set_warm_start(PartialSolution const& partial_solution);

  // SCIP requires to know in advance if the problem is to be solved
  // multiple times.
  void set_reoptimizing(bool);

  // SCIP requires this method to be called before every reoptimization.
  void setup_reoptimization();

  static std::map<Backend, std::string> backend_info();

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
  virtual ~ISolver() {}

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
  virtual double get_objective_value() const = 0;
  virtual Solver::Sense get_objective_sense() const = 0;

  virtual void add(Constr const& constr) = 0;
  virtual void add(IndicatorConstr const& constr) = 0;

  virtual void remove(Constr const& constr) = 0;

  virtual void add_lazy_constr_handler(LazyConstrHandler const& constr, bool at_integral_only) = 0;
  virtual std::pair<Solver::Result, bool> solve() = 0;
  virtual void set_non_convex_policy(Solver::NonConvexPolicy policy) = 0;
  virtual void set_indicator_constraint_policy(Solver::IndicatorConstraintPolicy policy);

  virtual void set_int_feasibility_tolerance(double value) = 0;
  virtual void set_feasibility_tolerance(double value) = 0;
  virtual void set_epsilon(double value) = 0;
  virtual void set_nr_threads(std::size_t) = 0;

  virtual double get_int_feasibility_tolerance() const = 0;
  virtual double get_feasibility_tolerance() const = 0;
  virtual double get_epsilon() const = 0;

  virtual bool supports_indicator_constraint(IndicatorConstr const& constr) const = 0;

  virtual bool supports_quadratic_constraints() const = 0;
  virtual bool supports_quadratic_objective() const = 0;

  virtual double infinity() const = 0;

  virtual void set_time_limit(double secs) = 0;

  virtual void dump(std::string const& filename) const = 0;

  virtual void set_warm_start(PartialSolution const& partial_solution) = 0;

  virtual void set_reoptimizing(bool) = 0;
  virtual void setup_reoptimization() = 0;

  Solver::IndicatorConstraintPolicy m_indicator_constraint_policy = 
    Solver::IndicatorConstraintPolicy::ReformulateIfUnsupported;
};

}  // namespace detail

std::ostream& operator<<(std::ostream& os, Solver::Backend const& solver_backend);

}  // namespace miplib
