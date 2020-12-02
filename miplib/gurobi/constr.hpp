#pragma once

#include <miplib/constr.hpp>

namespace miplib {

struct GurobiLinConstr : detail::IConstr
{
  GurobiLinConstr(
    Expr const& expr, Constr::Type const& type, std::optional<std::string> const& name
  ): detail::IConstr(expr, type, name)
  {}
  mutable std::optional<GRBConstr> m_constr;
};

struct GurobiQuadConstr : detail::IConstr
{
  GurobiQuadConstr(
    Expr const& expr, Constr::Type const& type, std::optional<std::string> const& name
  ):  detail::IConstr(expr, type, name)
  {}
  mutable std::optional<GRBQConstr> m_constr;
};

struct GurobiIndicatorConstr : detail::IIndicatorConstr
{
  GurobiIndicatorConstr(
    Constr const& implicant,
    Constr const& implicand,
    std::optional<std::string> const& name
  ): detail::IIndicatorConstr(implicant, implicand, name)
  {}
  mutable std::optional<GRBGenConstr> m_constr;
};

}  // namespace miplib
