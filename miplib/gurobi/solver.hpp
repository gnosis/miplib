#pragma once

#include <miplib/solver.hpp>

#include <gurobi_c++.h>

namespace miplib {

namespace detail {
struct GurobiCurrentStateHandle : GRBCallback, ICurrentStateHandle
{
  GurobiCurrentStateHandle(LazyConstrHandler const& constr_hdlr);
  double value(IVar const& var) const;
  void add_lazy(Constr const& constr);
  void callback();
  bool is_active() const { return m_active; }
  LazyConstrHandler m_constr_hdlr;
  bool m_active;
};
}

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

  void remove(Constr const& constr);

  void set_lazy_constr_handler(LazyConstrHandler const&);

  std::pair<Solver::Result, bool> solve();

  void set_non_convex_policy(Solver::NonConvexPolicy policy);
  void set_int_feasibility_tolerance(double value);
  void set_feasibility_tolerance(double value);

  double get_int_feasibility_tolerance() const;
  double get_feasibility_tolerance() const;

  void set_verbose(bool value);

  void set_pending_update() const;
  void update_if_pending() const;

  bool supports_quadratic_constraints() const { return true; }
  bool supports_quadratic_objective() const { return true; }

  bool supports_indicator_constraint(IndicatorConstr const& i) const;

  double infinity() const;

  void set_time_limit(double secs);

  void dump(std::string const& filename) const;

  bool is_in_callback() const;
  
  void set_warm_start(PartialSolution const& partial_solution);

  static std::string backend_info();

  GRBEnv env;
  mutable GRBModel model;
  mutable bool pending_update;
  std::unique_ptr<detail::GurobiCurrentStateHandle> p_callback;
};

}  // namespace miplib
