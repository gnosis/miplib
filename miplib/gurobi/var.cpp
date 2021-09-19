
#include "var.hpp"

namespace miplib {

GurobiVar::GurobiVar(Solver const& solver, GRBVar const& v): m_solver(solver), m_var(v)
{
  static_cast<GurobiSolver const&>(*m_solver.p_impl).set_pending_update();
}

void GurobiVar::update_solver_if_pending() const
{
  auto const& gurobi_solver = static_cast<GurobiSolver const&>(*m_solver.p_impl);
  if (!gurobi_solver.is_in_callback())
    gurobi_solver.update_if_pending();
}

double GurobiVar::value() const
{
  auto const& gurobi_solver = static_cast<GurobiSolver const&>(*m_solver.p_impl);

  if (gurobi_solver.is_in_callback())
    return gurobi_solver.p_callback->value(*this);

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
  auto const& gurobi_solver = static_cast<GurobiSolver const&>(*m_solver.p_impl);
  
  if (gurobi_solver.is_in_callback())
    throw std::logic_error("Operation not allowed within callback.");

  m_var.set(GRB_StringAttr_VarName, new_name);
  gurobi_solver.set_pending_update();  
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
  auto const& gurobi_solver = static_cast<GurobiSolver const&>(*m_solver.p_impl);
  
  if (gurobi_solver.is_in_callback())
    throw std::logic_error("Operation not allowed within callback.");

  m_var.set(GRB_DoubleAttr_LB, new_lb);
  gurobi_solver.set_pending_update();  
}

void GurobiVar::set_ub(double new_ub)
{
  auto const& gurobi_solver = static_cast<GurobiSolver const&>(*m_solver.p_impl);
  
  if (gurobi_solver.is_in_callback())
    throw std::logic_error("Operation not allowed within callback.");

  m_var.set(GRB_DoubleAttr_UB, new_ub);
  gurobi_solver.set_pending_update();  
}

void GurobiVar::set_start_value(double v)
{
  auto const& gurobi_solver = static_cast<GurobiSolver const&>(*m_solver.p_impl);
  
  if (gurobi_solver.is_in_callback())
    throw std::logic_error("Operation not allowed within callback.");

  m_var.set(GRB_DoubleAttr_Start, v);
  gurobi_solver.set_pending_update();
}

void GurobiVar::set_hint(double v)
{
  auto const& gurobi_solver = static_cast<GurobiSolver const&>(*m_solver.p_impl);
  
  if (gurobi_solver.is_in_callback())
    throw std::logic_error("Operation not allowed within callback.");

  m_var.set(GRB_DoubleAttr_VarHintVal, v);
  gurobi_solver.set_pending_update();
}

}  // namespace miplib
