
#include "var.hpp"

namespace miplib {

GurobiVar::GurobiVar(Solver const& solver, GRBVar const& v): m_solver(solver), m_var(v)
{
  static_cast<GurobiSolver const&>(*m_solver.p_impl).set_pending_update();
}

void GurobiVar::update_solver_if_pending() const
{
  static_cast<GurobiSolver const&>(*m_solver.p_impl).update_if_pending();
}

double GurobiVar::value() const
{
  update_solver_if_pending();
  return m_var.get(GRB_DoubleAttr_X);
}

Var::Type GurobiVar::type() const
{
  update_solver_if_pending();
  char vtype = m_var.get(GRB_CharAttr_VType);
  switch (vtype)
  {
    case 'C':
      return Var::Type::Continuous;
    case 'B':
      return Var::Type::Binary;
    case 'I':
      return Var::Type::Integer;
    default:
      throw std::logic_error("Gurobi variable type not handled yet.");
  }
}

std::optional<std::string> GurobiVar::name() const
{
  update_solver_if_pending();
  auto n = m_var.get(GRB_StringAttr_VarName);
  if (n.empty())
    return std::nullopt;
  return n;
}

void GurobiVar::set_name(std::string const& new_name)
{
  m_var.set(GRB_StringAttr_VarName, new_name);
  static_cast<GurobiSolver const&>(*m_solver.p_impl).set_pending_update();  
}

double GurobiVar::lb() const
{
  if (type() == Var::Type::Binary)
    return 0;
  update_solver_if_pending();
  return m_var.get(GRB_DoubleAttr_LB);
}

double GurobiVar::ub() const
{
  if (type() == Var::Type::Binary)
    return 1;
  update_solver_if_pending();
  return m_var.get(GRB_DoubleAttr_UB);
}

void GurobiVar::set_lb(double new_lb)
{
  m_var.set(GRB_DoubleAttr_LB, new_lb);
  static_cast<GurobiSolver const&>(*m_solver.p_impl).set_pending_update();  
}

void GurobiVar::set_ub(double new_ub)
{
  m_var.set(GRB_DoubleAttr_UB, new_ub);
  static_cast<GurobiSolver const&>(*m_solver.p_impl).set_pending_update();  
}

}  // namespace miplib
