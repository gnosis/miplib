#include <miplib/scip/solver.hpp>
#include <miplib/scip/util.hpp>

#include <miplib/scip/var.hpp>
#include <miplib/scip/constr.hpp>

#include <fmt/ostream.h>

#include <scip/scipdefplugins.h>

namespace miplib {


ScipSolver::ScipSolver(): p_env(nullptr), p_sol(nullptr), p_aux_obj_var(nullptr)
{
  SCIP_CALL_EXC(SCIPcreate(&p_env));
  SCIP_CALL_EXC(SCIPincludeDefaultPlugins(p_env));

  SCIP_CALL_EXC(SCIPcreateProbBasic(p_env, "unnamed"));
}


ScipSolver::~ScipSolver() noexcept(false)
{
  if (p_aux_obj_var != nullptr) {
    delete p_aux_obj_var;
  }
  SCIP_CALL_EXC(SCIPfree(&p_env));
}


std::shared_ptr<detail::IVar> ScipSolver::create_var(
  Solver const& solver,
  Var::Type const& type,
  std::optional<double> const& lb,
  std::optional<double> const& ub,
  std::optional<std::string> const& name
)
{
  return std::make_shared<ScipVar>(solver, type, lb, ub, name);
}

std::shared_ptr<detail::IConstr> ScipSolver::create_constr(
  Constr::Type const& type, Expr const& e, std::optional<std::string> const& name
)
{
  return std::make_shared<ScipConstr>(e, type, name);
}

std::shared_ptr<detail::IIndicatorConstr> ScipSolver::create_indicator_constr(
  Constr const& implicant,
  Constr const& implicand,
  std::optional<std::string> const& name
)
{
  if (implicant.type() != Constr::Equal)
  {
    throw std::logic_error("Scip indicator constraints require an equation implicant.");
  }

  if (!implicant.expr().is_linear())
  {
    throw std::logic_error("Scip indicator constraints require a linear implicant.");
  }

  auto implicant_linear_vars = implicant.expr().linear_vars();
  auto implicant_linear_coeffs = implicant.expr().linear_coeffs();

  if (implicant_linear_vars.size() != 1)
  {
    throw std::logic_error(
      "Scip indicator constraints require single variable implicant.");
  }

  double c = -implicant_linear_coeffs[0] * implicant.expr().constant();
  if (c != 0 and c != 1)
  {
    throw std::logic_error("Scip indicator constraints require a 'v = [0|1]' implicant.");
  }

  if (!implicand.expr().is_linear())
  {
    throw std::logic_error("Scip indicator constraints require a linear implicand.");
  }

  return std::make_shared<ScipIndicatorConstr>(implicant, implicand, name);
}

SCIP_CONS* ScipSolver::as_scip_constr(Constr const& constr)
{
  auto const& e = constr.expr();
  auto const& type = constr.type();
  auto const& name = constr.name();

  auto linear_coeffs = e.linear_coeffs();
  auto linear_vars = e.linear_vars();

  auto quad_coeffs = e.quad_coeffs();
  auto quad_vars_1 = e.quad_vars_1();
  auto quad_vars_2 = e.quad_vars_2();

  std::vector<SCIP_VAR*> scip_linear_vars;
  std::transform(
    linear_vars.begin(),
    linear_vars.end(),
    std::back_inserter(scip_linear_vars),
    [](auto const& v) { return static_cast<ScipVar const&>(*v.p_impl).p_var; }
  );

  std::vector<SCIP_VAR*> scip_quad_vars_1;
  std::transform(
    quad_vars_1.begin(),
    quad_vars_1.end(),
    std::back_inserter(scip_quad_vars_1),
    [](auto const& v) { return static_cast<ScipVar const&>(*v.p_impl).p_var; }
  );

  std::vector<SCIP_VAR*> scip_quad_vars_2;
  std::transform(
    quad_vars_2.begin(),
    quad_vars_2.end(),
    std::back_inserter(scip_quad_vars_2),
    [](auto const& v) { return static_cast<ScipVar const&>(*v.p_impl).p_var; }
  );

  SCIP_CONS* p_constr;

  if (e.is_linear())
  {
    SCIP_CALL_EXC(SCIPcreateConsBasicLinear(
      p_env,
      &p_constr,
      name.value_or("").c_str(),
      linear_coeffs.size(),
      scip_linear_vars.data(),
      linear_coeffs.data(),
      (type == Constr::Equal) ? -e.constant() : -SCIPinfinity(p_env),
      -e.constant()
    ));
  }
  else if (e.is_quadratic())
  {
    SCIP_CALL_EXC(SCIPcreateConsBasicQuadratic(
      p_env,
      &p_constr,
      name.value_or("").c_str(),
      linear_coeffs.size(),
      scip_linear_vars.data(),
      linear_coeffs.data(),
      quad_coeffs.size(),
      scip_quad_vars_1.data(),
      scip_quad_vars_2.data(),
      quad_coeffs.data(),
      (type == Constr::Equal) ? -e.constant() : -SCIPinfinity(p_env),
      -e.constant()
    ));
  }
  else
  {
    throw std::logic_error(fmt::format("SCIP does not support constraint {}.", constr));
  }
  return p_constr;
}


void ScipSolver::set_objective(Solver::Sense const& sense, Expr const& e)
{
  if (e.is_linear())
  {
    auto vars = e.linear_vars();
    auto coeffs = e.linear_coeffs();
    for (std::size_t i = 0; i < vars.size(); ++i)
    {
      auto p_scip_var = static_cast<ScipVar const&>(*vars[i].p_impl).p_var;
      SCIP_CALL_EXC(SCIPchgVarObj(p_env, p_scip_var, coeffs[i]));
    }
  }
  else
  if (e.is_quadratic())
  {
    // SCIP does not support non-linear objective functions directly. 
    // Reformulation using an auxilliary variable and a non-linear constraint:

    p_aux_obj_var = new Var(e.solver(), Var::Type::Continuous);
    add(e == *p_aux_obj_var);

    auto p_scip_var = static_cast<ScipVar const&>(*p_aux_obj_var->p_impl).p_var;
    SCIP_CALL_EXC(SCIPchgVarObj(p_env, p_scip_var, 1));
  }
  else
    assert(false);

	SCIP_OBJSENSE scip_sense = sense == Solver::Sense::Maximize
    ? SCIP_OBJSENSE_MAXIMIZE
    : SCIP_OBJSENSE_MINIMIZE;

  SCIP_CALL_EXC(SCIPsetObjsense(p_env, scip_sense));
}


void ScipSolver::add(Constr const& constr)
{
  auto const& constr_impl = static_cast<ScipConstr const&>(*constr.p_impl);

  if (constr_impl.p_constr != nullptr)
  {
    throw std::logic_error("Attempt to post the same constraint twice.");
  }

  SCIP_CONS* p_constr = as_scip_constr(constr);

  // add the constraint to scip
  SCIP_CALL_EXC(SCIPaddCons(p_env, p_constr));

  constr_impl.p_constr = p_constr;
}

void ScipSolver::add(IndicatorConstr const& constr)
{
  auto const& constr_impl = static_cast<ScipIndicatorConstr const&>(*constr.p_impl);

  if (constr_impl.p_constr_1 != nullptr)
  {
    std::logic_error("Attempt to post the same constraint twice.");
  }
  assert(constr_impl.p_constr_1 == nullptr);

  auto const& implicant = constr.implicant();
  auto const& implicand = constr.implicand();
  auto const& name = constr.name();

  assert(implicant.type() == Constr::Equal);
  assert(implicant.expr().is_linear());

  auto implicant_linear_vars = implicant.expr().linear_vars();
  auto implicant_linear_coeffs = implicant.expr().linear_coeffs();

  auto implicand_linear_vars = implicand.expr().linear_vars();
  auto implicand_linear_coeffs = implicand.expr().linear_coeffs();

  assert(implicant_linear_vars.size() == 1);

  double c = -implicant_linear_coeffs[0] * implicant.expr().constant();
  assert(c == 0 or c == 1);
  int bin_val = (int) c;
  SCIP_VAR* p_bin_var
    = static_cast<ScipVar const&>(*implicant_linear_vars[0].p_impl).p_var;

  // check whether we need the negated variable
  if (bin_val == 0)
  {
    SCIP_VAR* p_aux_var;
    SCIP_CALL_EXC(SCIPgetNegatedVar(p_env, p_bin_var, &p_aux_var));
    p_bin_var = p_aux_var;  // do we need to free anything?
    assert(p_bin_var != NULL);
  }

  assert(implicand.expr().is_linear());

  std::vector<SCIP_VAR*> scip_implicand_linear_vars;
  std::transform(
    implicand_linear_vars.begin(),
    implicand_linear_vars.end(),
    std::back_inserter(scip_implicand_linear_vars),
    [](auto const& v) { return static_cast<ScipVar const&>(*v.p_impl).p_var; }
  );

  SCIP_CONS* p_constr;
  SCIP_CALL_EXC(SCIPcreateConsBasicIndicator(
    p_env,
    &p_constr,
    name.value_or("").c_str(),
    p_bin_var,
    implicand_linear_vars.size(),
    scip_implicand_linear_vars.data(),
    implicand_linear_coeffs.data(),
    -implicand.expr().constant()
  ));

  // add constraint to scip
  SCIP_CALL_EXC(SCIPaddCons(p_env, p_constr));

  constr_impl.p_constr_1 = p_constr;

  if (implicand.type() == Constr::Equal)
  {
    std::vector<double> neg_coeffs;
    std::transform(
      implicand_linear_coeffs.begin(),
      implicand_linear_coeffs.end(),
      std::back_inserter(neg_coeffs),
      [](auto const& v) { return -v; }
    );

    SCIP_CONS* p_constr_2;
    SCIP_CALL_EXC(SCIPcreateConsBasicIndicator(
      p_env,
      &p_constr_2,
      name.value_or("").c_str(),
      p_bin_var,
      implicand_linear_vars.size(),
      scip_implicand_linear_vars.data(),
      neg_coeffs.data(),
      implicand.expr().constant()
    ));

    SCIP_CALL_EXC(SCIPaddCons(p_env, p_constr_2));
    constr_impl.p_constr_1 = p_constr_2;
  }
}

std::pair<Solver::Result, bool> ScipSolver::solve()
{
  auto nr_sols_before = SCIPgetNRuns(p_env) == 0 ? 0 : SCIPgetNBestSolsFound(p_env);

  SCIP_CALL_EXC(SCIPsolve(p_env));

  bool has_solution = SCIPgetNBestSolsFound(p_env) > nr_sols_before;

  // This will be 0 in case there is no solution.
  auto sol = SCIPgetBestSol(p_env);
  p_sol = (sol != 0) ? sol : nullptr;

  auto status = SCIPgetStatus(p_env);
  switch (status)
  {
    case SCIP_STATUS_OPTIMAL:
      return {Solver::Result::Optimal, true};

    case SCIP_STATUS_INFEASIBLE:
      return {Solver::Result::Infeasible, false};

    case SCIP_STATUS_INFORUNBD:
      return {Solver::Result::InfeasibleOrUnbounded, false};

    case SCIP_STATUS_UNBOUNDED:
      return {Solver::Result::Unbounded, false};

    case SCIP_STATUS_NODELIMIT:
    case SCIP_STATUS_TOTALNODELIMIT:
    case SCIP_STATUS_STALLNODELIMIT:
    case SCIP_STATUS_TIMELIMIT:
    case SCIP_STATUS_SOLLIMIT:
    case SCIP_STATUS_BESTSOLLIMIT:
    case SCIP_STATUS_MEMLIMIT:
    case SCIP_STATUS_GAPLIMIT:
    case SCIP_STATUS_USERINTERRUPT:
    case SCIP_STATUS_RESTARTLIMIT:
      return {Solver::Result::Interrupted, has_solution};

    case SCIP_STATUS_TERMINATE:
      return {Solver::Result::Error, has_solution};

    default:
      return {Solver::Result::Other, has_solution};
  }
}

void ScipSolver::set_non_convex_policy(Solver::NonConvexPolicy /*policy*/)
{
  // It seems scip doesn't require to set any flags.
}

void ScipSolver::set_verbose(bool value)
{
  SCIPmessagehdlrSetQuiet(SCIPgetMessagehdlr(p_env), !value);
}

double ScipSolver::infinity() const
{
  return SCIPinfinity(p_env);
}

void ScipSolver::dump(std::string const& filename) const
{
  SCIP_CALL_EXC(SCIPwriteOrigProblem(p_env, filename.c_str(), NULL, false));	
}

}  // namespace miplib