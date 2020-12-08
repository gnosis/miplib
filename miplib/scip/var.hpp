
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
  void set_name(std::string const& new_name);

  Solver const& solver() const
  {
    return m_solver;
  }

  double lb() const;
  double ub() const;

  void set_lb(double new_lb);
  void set_ub(double new_ub);

  Solver m_solver;
  SCIP_VAR* p_var;
};

}  // namespace miplib
