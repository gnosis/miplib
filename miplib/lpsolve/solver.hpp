#pragma once

#include <miplib/solver.hpp>

#include <lpsolve/lp_lib.h>

namespace miplib {

struct LpsolveSolver : detail::ISolver
{
  LpsolveSolver();
  virtual ~LpsolveSolver() noexcept(false);

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

  void remove(Constr const& constr);

  void set_lazy_constr_handler(LazyConstrHandler const&) { throw std::logic_error("Not implemented yet."); }

  std::pair<Solver::Result, bool> solve();

  void set_non_convex_policy(Solver::NonConvexPolicy policy);

  void set_int_feasibility_tolerance(double value);
  void set_feasibility_tolerance(double value);

  double get_int_feasibility_tolerance() const;
  double get_feasibility_tolerance() const;

  void set_verbose(bool value);

  std::vector<int> get_col_idxs(std::vector<Var> const& vars);

  bool supports_indicator_constraint(IndicatorConstr const& constr) const;

  bool supports_quadratic_constraints() const { return false; }
  bool supports_quadratic_objective() const { return false; }

  double infinity() const;

  void set_time_limit(double secs);

  void dump(std::string const& filename) const;

  void set_warm_start(PartialSolution const& partial_solution);

  static std::string backend_info();

  lprec* p_lprec;
  std::vector<double> m_last_solution;
};

}  // namespace miplib
