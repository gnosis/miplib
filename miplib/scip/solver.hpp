#pragma once

#include <miplib/solver.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <scip/scip.h>

#pragma GCC diagnostic pop

namespace miplib {

struct LazyConstrHandler;
struct ScipSolver;

namespace detail {
struct ScipCurrentStateHandle : ICurrentStateHandle
{
  ScipCurrentStateHandle(ScipSolver& solver, SCIP_SOL* ap_sol);
  virtual ~ScipCurrentStateHandle() {}

  double value(IVar const& var) const;
  void add_lazy(Constr const& constr);
  bool is_active() const { return m_active; }

  ScipSolver& m_solver;
  SCIP_SOL* p_sol;
  bool m_active;
};
}

struct ScipSolver : detail::ISolver
{
  ScipSolver();
  virtual ~ScipSolver();

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
  double get_objective_value() const;

  void add(Constr const& constr);
  void add(IndicatorConstr const& constr);

  void remove(Constr const& constr);
  
  void add_lazy_constr_handler(LazyConstrHandler const& constr, bool at_integral_nodes_only);

  std::pair<Solver::Result, bool> solve();

  void set_non_convex_policy(Solver::NonConvexPolicy policy);
  void set_int_feasibility_tolerance(double value);
  void set_feasibility_tolerance(double value);
  void set_epsilon(double value);
  void set_nr_threads(std::size_t nr_threads);

  double get_int_feasibility_tolerance() const;
  double get_feasibility_tolerance() const;
  double get_epsilon() const;

  void set_verbose(bool value);

  SCIP_CONS* as_scip_constr(Constr const& constr);

  bool supports_indicator_constraint(IndicatorConstr const& constr) const;

  bool supports_quadratic_constraints() const { return true; }
  bool supports_quadratic_objective() const { return true; }

  double infinity() const;

  void dump(std::string const& filename) const;

  void set_time_limit(double secs);

  bool is_in_callback() const;

  void set_warm_start(PartialSolution const& partial_solution);

  void set_reoptimizing(bool);
  void setup_reoptimization();

  static std::string backend_info();

  static bool is_available();

  SCIP* p_env;
  SCIP_SOL* p_sol;
  Var* p_aux_obj_var;
  std::unique_ptr<detail::ScipCurrentStateHandle> p_current_state_handler;
};

}  // namespace miplib
