
#pragma once

#include <miplib/var.hpp>
#include <miplib/lpsolve/solver.hpp>

namespace miplib {

struct LpsolveVar : detail::IVar
{
  LpsolveVar(
    Solver const& solver,
    Var::Type const& type,
    std::optional<double> const& lb,
    std::optional<double> const& ub,
    std::optional<std::string> const& name
  );

  double value() const;

  Var::Type type() const;

  std::optional<std::string> name() const;

  Solver const& solver() const
  {
    return m_solver;
  }

  int cur_col_idx() const;

  Solver m_solver;
  int m_orig_col_idx;
};

}  // namespace miplib
