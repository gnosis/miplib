#include "constr.hpp"
#include "solver.hpp"

namespace miplib {

Constr::Constr(
  Solver const& solver,
  Constr::Type const& type,
  Expr const& e,
  std::optional<std::string> const& name):
  p_impl(solver.p_impl->create_constr(type, e, name)
)
{}

Expr Constr::expr() const
{
  return p_impl->m_expr;
}

Constr::Type Constr::type() const
{
  return p_impl->m_type;
}

std::optional<std::string> const& Constr::name() const
{
  return p_impl->m_name;
}

std::ostream& operator<<(std::ostream& os, Constr const& c)
{
  os << c.expr() << " ";
  os << ((c.type() == Constr::LessEqual) ? "<= " : "= ");
  os << 0;
  return os;
}

Constr operator!(Expr const& e)
{
  if (!e.must_be_binary())
  {
    throw std::logic_error("Attempt to negate a possibly non-binary expression");
  }
  return e == 0;
}

Constr operator<=(Expr const& e1, Expr const& e2)
{
  Solver const* p_solver;
  if (!e1.is_constant())
    p_solver = &e1.solver();
  else if (!e2.is_constant())
    p_solver = &e2.solver();
  else
  {
    throw std::logic_error("Attempt to create a constraint from constant expressions");
  }
  return Constr(*p_solver, Constr::LessEqual, e1 - e2);
}

Constr operator==(Expr const& e1, Expr const& e2)
{
  Solver const* p_solver;
  if (!e1.is_constant())
    p_solver = &e1.solver();
  else if (!e2.is_constant())
    p_solver = &e2.solver();
  else
  {
    throw std::logic_error("Attempt to create a constraint from constant expressions");
  }
  return Constr(*p_solver, Constr::Equal, e1 - e2);
}


IndicatorConstr::IndicatorConstr(
  Solver const& solver,
  Constr const& implicant,
  Constr const& implicand,
  std::optional<std::string> const& name
):
  p_impl(solver.p_impl->create_indicator_constr(implicant, implicand, name))
{}

Constr const& IndicatorConstr::implicant() const
{
  return p_impl->m_implicant;
}

Constr const& IndicatorConstr::implicand() const
{
  return p_impl->m_implicand;
}

std::optional<std::string> const& IndicatorConstr::name() const
{
  return p_impl->m_name;
}

std::ostream& operator<<(std::ostream& os, IndicatorConstr const& ic)
{
  os << ic.implicant() << " -> " << ic.implicand();
  return os;
}

IndicatorConstr operator>>(Constr const& implicant, Constr const& implicand)
{
  return IndicatorConstr(implicand.expr().solver(), implicant, implicand);
}

IndicatorConstr operator<<(Constr const& implicand, Constr const& implicant)
{
  return IndicatorConstr(implicand.expr().solver(), implicant, implicand);
}

}  // namespace miplib
