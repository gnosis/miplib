#pragma once

#include <miplib/gurobi/solver.hpp>
#include <miplib/var.hpp>

namespace miplib {

struct GurobiVar : detail::IVar
{
  GurobiVar(Solver const& solver, GRBVar const& v);
  virtual ~GurobiVar() {};

  void update_solver_if_pending() const;

  double value() const;

  Var::Type type() const;

  std::optional<std::string> name() const;
  void set_name(std::string const& new_name);

  Solver const& solver() const { return m_solver; }

  double lb() const;
  double ub() const;

  void set_lb(double new_lb);
  void set_ub(double new_ub);

  void set_start_value(double v);
  void set_hint(double v);

  Solver m_solver;
  GRBVar m_var;
};

}  // namespace miplib
