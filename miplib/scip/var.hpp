
#pragma once

#include <miplib/var.hpp>
#include <miplib/scip/solver.hpp>

namespace miplib {

struct ScipVar : detail::IVar
{
  ScipVar(
    Solver const& solver,
    Var::Type const& type,
    std::optional<double> const& lb,
    std::optional<double> const& ub,
    std::optional<std::string> const& name
  );

  virtual ~ScipVar() noexcept(false);

  double value() const;

  Var::Type type() const;

  std::optional<std::string> name() const;

  Solver const& solver() const
  {
    return m_solver;
  }

  Solver m_solver;
  SCIP_VAR* p_var;
};

}  // namespace miplib
