#pragma once

#include <miplib/gurobi/solver.hpp>
#include <miplib/var.hpp>

namespace miplib {

struct GurobiVar : detail::IVar
{
  GurobiVar(Solver const& solver, GRBVar const& v);

  void update_solver_if_pending() const;

  double value() const;

  Var::Type type() const;

  std::optional<std::string> name() const;

  Solver const& solver() const
  {
    return m_solver;
  }
  Solver m_solver;
  GRBVar const m_var;
};

}  // namespace miplib
