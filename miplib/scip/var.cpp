#include <miplib/scip/var.hpp>

#include <miplib/scip/util.hpp>

namespace miplib {

ScipVar::ScipVar(
  Solver const& solver,
  Var::Type const& type,
  std::optional<double> const& lb,
  std::optional<double> const& ub,
  std::optional<std::string> const& name
):
  m_solver(solver),
  p_var(nullptr)
{
  auto p_env = static_cast<ScipSolver const&>(*m_solver.p_impl).p_env;

  SCIP_VARTYPE scip_var_type;
  double scip_lb, scip_ub;

  if (type == Var::Type::Continuous)
  {
    scip_var_type = SCIP_VARTYPE_CONTINUOUS;
    scip_lb = lb.value_or(-SCIPinfinity(p_env));
    scip_ub = ub.value_or(SCIPinfinity(p_env));
  }
  else if (type == Var::Type::Binary)
  {
    scip_var_type = SCIP_VARTYPE_BINARY;
    if (lb.value_or(0) != 0 or ub.value_or(1) != 1)
    {
      throw std::logic_error("Binary variables bounds must be 0..1.");
    }
    scip_lb = 0;
    scip_ub = 1;
  }
  else if (type == Var::Type::Integer)
  {
    scip_var_type = SCIP_VARTYPE_INTEGER;
    scip_lb = lb.value_or(-SCIPinfinity(p_env));
    scip_ub = ub.value_or(SCIPinfinity(p_env));
  }
  else
  {
    throw std::logic_error("SCIP does not support this variable type");
  }

  SCIP_CALL_EXC(SCIPcreateVarBasic(
    p_env,
    &p_var,
    name.has_value() ? name.value().c_str() : NULL,
    scip_lb,
    scip_ub,
    0.0,  // obj
    scip_var_type
  ));

  // add the SCIP_VAR object to the scip problem
  SCIP_CALL_EXC(SCIPaddVar(p_env, p_var));
}

ScipVar::~ScipVar() noexcept(false)
{
  auto p_env = static_cast<ScipSolver const&>(*m_solver.p_impl).p_env;
  SCIP_CALL_EXC(SCIPreleaseVar(p_env, &p_var));
}

double ScipVar::value() const
{
  auto const& scip_solver = static_cast<ScipSolver const&>(*m_solver.p_impl);
  SCIP_SOL* p_sol = scip_solver.p_sol;
  if (p_sol == nullptr)
  {
    throw std::logic_error(
      "Attempt to access value of variable before a solution was found."
    );
  }
  return SCIPgetSolVal(scip_solver.p_env, p_sol, p_var);
}

Var::Type ScipVar::type() const
{
  SCIP_VARTYPE scip_type = SCIPvarGetType(p_var);

  switch (scip_type)
  {
    case SCIP_VARTYPE_CONTINUOUS:
      return Var::Type::Continuous;
    case SCIP_VARTYPE_BINARY:
      return Var::Type::Binary;
    case SCIP_VARTYPE_INTEGER:
      return Var::Type::Integer;
    default:
      throw std::logic_error("SCIP variable type not handled yet.");
  }
}

std::optional<std::string> ScipVar::name() const
{
  std::string name = SCIPvarGetName(p_var);
  if (name.empty())
    return std::nullopt;
  return name;
}

void ScipVar::set_name(std::string const& new_name)
{
  auto p_env = static_cast<ScipSolver const&>(*m_solver.p_impl).p_env;  
  SCIPchgVarName(p_env, p_var, new_name.c_str());
}

double ScipVar::lb() const
{
  if (type() == Var::Type::Binary)
    return 0;
  return SCIPvarGetLbOriginal(p_var)	;
}

double ScipVar::ub() const
{
  if (type() == Var::Type::Binary)
    return 1;
  return SCIPvarGetUbOriginal(p_var)	;
}

void ScipVar::set_lb(double new_lb)
{
  auto p_env = static_cast<ScipSolver const&>(*m_solver.p_impl).p_env;
  SCIPchgVarLb(p_env, p_var, new_lb);
}

void ScipVar::set_ub(double new_ub)
{
  auto p_env = static_cast<ScipSolver const&>(*m_solver.p_impl).p_env;
  SCIPchgVarUb(p_env, p_var, new_ub);
}

}  // namespace miplib
