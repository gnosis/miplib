#pragma once

#include <miplib/scip/solver.hpp>
#include <miplib/scip/util.hpp>

namespace miplib {

struct ScipConstr : detail::IConstr
{
  ScipConstr(
    Expr const& expr, Constr::Type const& type, std::optional<std::string> const& name
  ):
    detail::IConstr(expr, type, name),
    m_solver(expr.solver()), p_constr(nullptr)
  {}

  virtual ~ScipConstr() noexcept(false)
  {
    if (p_constr == nullptr)
      return;

    auto p_env = static_cast<ScipSolver const&>(*m_solver.p_impl).p_env;

    auto scip_stage = SCIPgetStage(p_env);
    if(scip_stage != SCIP_STAGE_PROBLEM and scip_stage != SCIP_STAGE_SOLVING)
      SCIP_CALL_EXC(SCIPfreeTransform(p_env));

    SCIP_CALL_EXC(SCIPreleaseCons(p_env, &p_constr));
  }

  Solver m_solver;
  mutable SCIP_CONS* p_constr;
};


struct ScipIndicatorConstr : detail::IIndicatorConstr
{
  ScipIndicatorConstr(
    Constr const& implicant,
    Constr const& implicand,
    std::optional<std::string> const& name
  ):
    detail::IIndicatorConstr(implicant, implicand, name),
    m_solver(implicant.expr().solver()), p_constr_1(nullptr), p_constr_2(nullptr)
  {}

  virtual ~ScipIndicatorConstr() noexcept(false)
  {
    auto p_env = static_cast<ScipSolver const&>(*m_solver.p_impl).p_env;
    if (p_constr_1 != nullptr)
    {
      SCIP_CALL_EXC(SCIPreleaseCons(p_env, &p_constr_1));
    }
    if (p_constr_2 != nullptr)
    {
      SCIP_CALL_EXC(SCIPreleaseCons(p_env, &p_constr_2));
    }
  }

  Solver m_solver;
  // indicator constraints in scip are of the form v -> (a <= b)
  // and therefore we need 2 for enforcing v -> (a == b).
  mutable SCIP_CONS* p_constr_1;
  mutable SCIP_CONS* p_constr_2;
};


}  // namespace miplib