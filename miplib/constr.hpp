#pragma once

#include "expr.hpp"
#include "util.hpp"

#include <memory>
#include <iostream>

namespace miplib {

namespace detail {
struct IConstr;
struct IIndicatorConstr;
}  // namespace detail

// Linear or quadratic constraint.
struct Constr
{
  enum Type { LessEqual, Equal };

  Constr(
    Solver const& solver,
    Constr::Type const& type,
    Expr const& e,
    std::optional<std::string> const& name = std::nullopt
  );

  Expr expr() const;
  Type type() const;
  std::optional<std::string> const& name() const;

  private:
  std::shared_ptr<detail::IConstr> p_impl;
  friend std::ostream& operator<<(std::ostream& os, Constr const& c);
  friend struct GurobiSolver;
  friend struct ScipSolver;
  friend struct LpsolveSolver;
};

Constr operator!(Expr const& e);

Constr operator<=(Expr const& e1, Expr const& e2);
Constr operator==(Expr const& e1, Expr const& e2);

inline Constr operator>=(Expr const& e1, Expr const& e2)
{
  return e2 <= e1;
}

// Indicator constraint.
struct IndicatorConstr
{
  // implicant -> implicand
  IndicatorConstr(
    Solver const& solver,
    Constr const& implicant,
    Constr const& implicand,
    std::optional<std::string> const& name = std::nullopt
  );

  Constr const& implicant() const;
  Constr const& implicand() const;
  std::optional<std::string> const& name() const;

  private:
  std::shared_ptr<detail::IIndicatorConstr> p_impl;
  friend std::ostream& operator<<(std::ostream& os, IndicatorConstr const& c);
  friend struct GurobiSolver;
  friend struct ScipSolver;
};

std::ostream& operator<<(std::ostream& os, Expr const& e);

IndicatorConstr operator>>(Constr const& implicant, Constr const& implicand);
IndicatorConstr operator<<(Constr const& implicand, Constr const& implicant);

inline IndicatorConstr operator>>(Expr const& implicant, Constr const& implicand)
{
  return (implicant == 1) >> implicand;
}

inline IndicatorConstr operator<<(Constr const& implicand, Expr const& implicant)
{
  return (implicant == 1) >> implicand;
}

namespace detail {
struct IConstr
{
  IConstr(
    Expr const& expr, Constr::Type const& type, std::optional<std::string> const& name):
    m_expr(expr),
    m_type(type), m_name(name)
  {}
  Expr m_expr;
  Constr::Type m_type;
  std::optional<std::string> m_name;
};

struct IIndicatorConstr
{
  IIndicatorConstr(
    Constr const& implicant,
    Constr const& implicand,
    std::optional<std::string> const& name):
    m_implicant(implicant),
    m_implicand(implicand), m_name(name)
  {}
  Constr m_implicant;
  Constr m_implicand;
  std::optional<std::string> m_name;
};

}  // namespace detail

}  // namespace miplib
