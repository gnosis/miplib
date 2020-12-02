#pragma once

#include <miplib/scip/solver.hpp>
#include <miplib/scip/util.hpp>

namespace miplib {

struct LpsolveConstr : detail::IConstr
{
  LpsolveConstr(
    Expr const& expr, Constr::Type const& type, std::optional<std::string> const& name
  ):
    detail::IConstr(expr, type, name),
    m_solver(expr.solver()), m_orig_row_idx(-1)
  {}

  Solver m_solver;
  mutable int m_orig_row_idx;
};



}  // namespace miplib