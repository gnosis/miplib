#include "constr.hpp"
#include "solver.hpp"
#include <miplib/util/scale.hpp>

namespace miplib {

Constr::Constr(
  Solver const& solver,
  Constr::Type const& type,
  Expr const& e,
  std::optional<std::string> const& name):
  p_impl(solver.p_impl->create_constr(type, e, name)
)
{
  if (type == Type::Equal)
  {
    if (e.ub() < 0 or e.lb() > 0)
      throw std::logic_error("Attempt to create a constraint that is trivially unsat.");
  }
  else
  if (type == Type::LessEqual)
  {
    if (e.lb() > 0)
      throw std::logic_error("Attempt to create a constraint that is trivially unsat.");
  }
}

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

// If the truth value of the constraint can be captured as
// a linear expression (without introducing extra variables).
bool Constr::is_reifiable() const
{
  auto const& e = expr();
  if (!e.is_linear())
    return false;
  if (e.arity() != 1)
    return false;
  if (e.linear_vars()[0].type() != Var::Type::Binary)
    return false;
  double d = e.constant() / e.linear_coeffs()[0];
  return d == 0 or d == 1 or d == -1;
}

// Get the truth value of the constraint as a linear expression.
Expr Constr::reified() const
{
  if (!is_reifiable())
    throw std::logic_error("Attempt to reify non-reifiable constraint.");
  auto const& e = expr();
  double d = e.constant() / e.linear_coeffs()[0];
  if (d == 0)
    return e.linear_vars()[0];
  else
  {
    assert(d == 1 or d == -1);
    return 1 - e.linear_vars()[0];
  }
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

Constr Constr::scale(double skip_lb, double skip_ub) const
{
  return detail::scale_gm(*this, skip_lb, skip_ub);
}

/**
 *  Indicator constraints
 **/

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

namespace detail {
std::shared_ptr<detail::IIndicatorConstr> create_reformulatable_indicator_constr(
  Constr const& implicant,
  Constr const& implicand,
  std::optional<std::string> const& name
)
{
  if (implicant.type() != Constr::Equal)
  {
    throw std::logic_error("Indicator constraints require an equation implicant.");
  }

  if (!implicant.expr().is_linear())
  {
    throw std::logic_error("Indicator constraints require a linear implicant.");
  }

  auto implicant_linear_vars = implicant.expr().linear_vars();
  auto implicant_linear_coeffs = implicant.expr().linear_coeffs();

  if (implicant_linear_vars.size() != 1)
  {
    throw std::logic_error(
      "Indicator constraints require single variable implicant.");
  }

  double c = -implicant_linear_coeffs[0] * implicant.expr().constant();
  if (c != 0 and c != 1)
  {
    throw std::logic_error("Indicator constraints require a 'v = [0|1]' implicant.");
  }

  if (!implicand.expr().is_linear())
  {
    throw std::logic_error("Indicator constraints require a linear implicand.");
  }

  return std::make_shared<IIndicatorConstr>(implicant, implicand, name);
}
} // namespace detail

bool IndicatorConstr::has_reformulation() const
{
  if (!implicant().is_reifiable())
    return false;
  auto const& solver = implicand().expr().solver();

  if (implicand().expr().ub() == solver.infinity())
    return false;
  
  if (implicand().type() == Constr::Type::LessEqual)
    return true;
  
  assert(implicand().type() == Constr::Type::Equal);
  if (implicand().expr().lb() == -solver.infinity())
    return false;

  return true;
}


std::vector<Constr> IndicatorConstr::reformulation() const
{
  if (!implicant().is_reifiable())
    throw std::logic_error(
      "Attempt to reformulate indicator constraint with non reifiable implicant."
    );

  auto const& solver = implicand().expr().solver();

  double ub = implicand().expr().ub();
  if (ub == solver.infinity())
    throw std::logic_error(
      "Attempt to reformulate indicator constraint with unknown implicand upper bound."
      " Try bounding the domain of the involved variables."
    );

  std::vector<Constr> r;

  // z = 1 -> LinExpr <= 0
  // <-> LinExpr - ub(LinExpr) * (1-z) <= 0

  // add ub reformulation if not redundant
  if (ub > 0) 
    r.push_back(implicand().expr() <= ub * implicant().reified());

  if (implicand().type() == Constr::Type::LessEqual)
    return r;

  assert(implicand().type() == Constr::Type::Equal);

  double lb = implicand().expr().lb();
  if (lb == -solver.infinity())
    throw std::logic_error(
      "Attempt to reformulate indicator constraint with unknown implicand lower bound."
      " Try bounding the domain of the involved variables."
    );

  // z = 1 -> LinExpr = 0
  // <-> z = 1 -> LinExpr <= 0 /\ z = 1 -> -LinExpr <= 0
  // <-> LinExpr - ub(LinExpr) * (1-z) <= 0 /\ -LinExpr - ub(-LinExpr) * (1-z) <= 0
  // <-> LinExpr - ub(LinExpr) * (1-z) <= 0 /\ -LinExpr + lb(LinExpr) * (1-z) <= 0

  if (lb < 0) 
    r.push_back(lb * implicant().reified() <= implicand().expr());

  return r;
}

std::vector<Constr> IndicatorConstr::scale(double skip_lb, double skip_ub) const
{
  std::vector<Constr> r;
  for (auto const& ctr : reformulation())
    r.push_back(ctr.scale(skip_lb, skip_ub));
  return r;
}

}  // namespace miplib
