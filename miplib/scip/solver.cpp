#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <miplib/scip/solver.hpp>
#include <miplib/scip/util.hpp>

#include <miplib/scip/var.hpp>
#include <miplib/scip/constr.hpp>
#include <miplib/lazy.hpp>

#include <fmt/ostream.h>

#include <scip/scipdefplugins.h>
#include <objscip/objconshdlr.h>

#include <spdlog/spdlog.h>

namespace miplib {


ScipSolver::ScipSolver(bool verbose): p_env(nullptr), p_sol(nullptr), p_aux_obj_var(nullptr)
{
  SCIP_CALL_EXC(SCIPcreate(&p_env));
  SCIP_CALL_EXC(SCIPincludeDefaultPlugins(p_env));

  SCIP_CALL_EXC(SCIPcreateProbBasic(p_env, "unnamed"));
  //SCIP_CALL_EXC(SCIPsetPresolving(p_env, SCIP_PARAMSETTING_OFF, true));

  set_verbose(verbose);
}


ScipSolver::~ScipSolver()
{
  if (p_aux_obj_var != nullptr) {
    delete p_aux_obj_var;
  }
  SCIP_CALL_TERM(SCIPfree(&p_env));
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
    SCIP_CALL_EXC(SCIPcreateConsBasicQuadraticNonlinear(
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

double ScipSolver::get_objective_value() const
{
  return SCIPgetPrimalbound(p_env);
}

Solver::Sense ScipSolver::get_objective_sense() const
{
  auto scip_sense = SCIPgetObjsense(p_env);
  if (scip_sense == SCIP_OBJSENSE_MINIMIZE)
    return Solver::Sense::Minimize;
  else
  {
    assert(scip_sense == SCIP_OBJSENSE_MAXIMIZE);
    return Solver::Sense::Maximize;
  }
}

void ScipSolver::add(Constr const& constr)
{
  if (is_in_callback())
  {
    p_current_state_handler->add_lazy(constr);
    return;
  }

  auto const& constr_impl = static_cast<ScipConstr const&>(*constr.p_impl);

  if (constr_impl.p_constr != nullptr)
  {
    throw std::logic_error("Attempt to post the same constraint twice.");
  }

  if(SCIPgetStage(p_env) == SCIP_STAGE_SOLVED)
    setup_reoptimization();

  SCIP_CONS* p_constr = as_scip_constr(constr);

  // add the constraint to scip
  SCIP_CALL_EXC(SCIPaddCons(p_env, p_constr));

  constr_impl.p_constr = p_constr;
}

bool ScipSolver::supports_indicator_constraint(IndicatorConstr const& constr) const
{
  auto const& implicant = constr.implicant();
  auto const& implicand = constr.implicand();

  // Scip indicator constraints require an equation implicant.
  if (implicant.type() != Constr::Equal)
    return false;

  // Scip indicator constraints require a linear implicant.
  if (!implicant.expr().is_linear())
    return false;

  auto implicant_linear_vars = implicant.expr().linear_vars();
  auto implicant_linear_coeffs = implicant.expr().linear_coeffs();

  // Scip indicator constraints require single variable implicant.
  if (implicant_linear_vars.size() != 1)
    return false;

  // Scip indicator constraints require a 'v = [0|1]' implicant.
  double c = -implicant_linear_coeffs[0] * implicant.expr().constant();
  if (c != 0 and c != 1)
    return false;

  // Scip indicator constraints require a linear implicand.
  if (!implicand.expr().is_linear())
    return false;
  
  return true;
}


void ScipSolver::add(IndicatorConstr const& constr)
{
  auto const& constr_impl = static_cast<ScipIndicatorConstr const&>(*constr.p_impl);
  
  if (constr_impl.p_constr_1 != nullptr)
  {
    std::logic_error("Attempt to post the same constraint twice.");
  }
  assert(constr_impl.p_constr_1 == nullptr);

  if (!supports_indicator_constraint(constr))
    throw std::logic_error(
      "Scip does not support this indicator constraint. Try .reformulation()."
    );

  if(SCIPgetStage(p_env) == SCIP_STAGE_SOLVED)
    setup_reoptimization();

  auto const& implicant = constr.implicant();
  auto const& implicand = constr.implicand();
  auto const& name = constr.name();

  auto implicant_linear_vars = implicant.expr().linear_vars();
  auto implicant_linear_coeffs = implicant.expr().linear_coeffs();


  assert(implicant.type() == Constr::Equal);
  assert(implicant.expr().is_linear());

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

void ScipSolver::remove(Constr const& constr)
{
  if(SCIPgetStage(p_env) == SCIP_STAGE_SOLVED)
    SCIP_CALL_EXC(SCIPfreeTransform(p_env));
 
  auto p_scip_constr = static_cast<ScipConstr const&>(*constr.p_impl).p_constr;
  SCIP_CALL_EXC(SCIPdelCons(p_env, p_scip_constr));
}

std::pair<Solver::Result, bool> ScipSolver::solve()
{
  SCIP_CALL_EXC(SCIPsolve(p_env));

  bool has_solution = SCIPgetNBestSolsFound(p_env) > 0;

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

void ScipSolver::set_feasibility_tolerance(double value)
{
  SCIP_CALL_EXC(SCIPsetRealParam(p_env, "numerics/feastol", value));	
}

void ScipSolver::set_int_feasibility_tolerance(double value)
{
  set_feasibility_tolerance(value);
}

void ScipSolver::set_epsilon(double value)
{
  SCIP_CALL_EXC(SCIPsetRealParam(p_env, "numerics/epsilon", value));
  SCIP_CALL_EXC(SCIPsetRealParam(p_env, "numerics/sumepsilon", value));
}

void ScipSolver::set_nr_threads(std::size_t nr_threads)
{
  SCIP_CALL_EXC(SCIPsetIntParam(p_env, "parallel/maxnthreads", nr_threads));
}

double ScipSolver::get_feasibility_tolerance() const
{
  double v = 0;
  SCIP_CALL_EXC(SCIPgetRealParam(p_env, "numerics/feastol", &v));
  return v;
}

double ScipSolver::get_int_feasibility_tolerance() const
{
  return get_feasibility_tolerance();
}

double ScipSolver::get_epsilon() const
{
  double v = 0;
  SCIP_CALL_EXC(SCIPgetRealParam(p_env, "numerics/epsilon", &v));
  return v;
}

double ScipSolver::infinity() const
{
  return SCIPinfinity(p_env);
}

void ScipSolver::set_time_limit(double secs)
{
  SCIP_CALL_EXC(SCIPsetRealParam(p_env, "limits/time", secs));
}

void ScipSolver::dump(std::string const& filename) const
{
  SCIP_CALL_EXC(SCIPwriteOrigProblem(p_env, filename.c_str(), NULL, false));	
}

bool ScipSolver::is_in_callback() const
{
  return (bool)p_current_state_handler;
}

void ScipSolver::set_warm_start(PartialSolution const& partial_solution)
{
  SCIP_SOL* p_sol;
  SCIP_CALL_EXC(SCIPcreatePartialSol(p_env, &p_sol, NULL));

  for (auto const& [var, val] : partial_solution)
  {
    auto p_scip_var = static_cast<ScipVar const&>(*var.p_impl).p_var;
    SCIP_CALL_EXC(SCIPsetSolVal(p_env, p_sol, p_scip_var, val));
  }

  unsigned int stored;
  SCIP_CALL_EXC(SCIPaddSolFree(p_env, &p_sol, &stored));
  if (!stored)
    spdlog::warn("Warm start solution was ignored.");
}

void ScipSolver::set_reoptimizing(bool value)
{
  SCIP_CALL_EXC(SCIPenableReoptimization(p_env, value)); 
}	

void ScipSolver::setup_reoptimization()
{
  //SCIP_CALL_EXC(SCIPfreeReoptSolve(p_env));
  SCIP_CALL_EXC(SCIPfreeTransform(p_env));
}

namespace detail {

ScipCurrentStateHandle::ScipCurrentStateHandle(ScipSolver& solver, SCIP_SOL* ap_sol) : 
  m_solver(solver), p_sol(ap_sol), m_active(false) 
{}
  
double ScipCurrentStateHandle::value(IVar const& var) const
{ 
  auto p_env = m_solver.p_env;
  auto p_var = static_cast<ScipVar const&>(var).p_var;
  return SCIPgetSolVal(p_env, p_sol, p_var);
}

void ScipCurrentStateHandle::add_lazy(Constr const& constr)
{
  auto const& constr_impl = static_cast<ScipConstr const&>(*constr.p_impl);

  if (constr_impl.p_constr != nullptr)
  {
    throw std::logic_error("Attempt to post the same constraint twice.");
  }

  SCIP_CONS* p_constr = m_solver.as_scip_constr(constr);

  // add the constraint to scip
  SCIP_CALL_EXC(SCIPaddCons(m_solver.p_env, p_constr));

  constr_impl.p_constr = p_constr;
}

} // namespace detail


struct ScipConstraintHandler: scip::ObjConshdlr
{
  static std::size_t nr_instances;
  std::string make_new_name()
  {
    return "ScipConstraintHandler_" + std::to_string(nr_instances++);
  }

  ScipConstraintHandler(
    ScipSolver& solver,
    LazyConstrHandler const& constr_hdlr,
    int enforcing_priority,
    int checking_priority
  ) : 
    scip::ObjConshdlr(
      solver.p_env,
      make_new_name().c_str(),// name
      "",     // description
      0,      // priority for separation
      enforcing_priority,     // priority for constraint enforcing. <0 means integral solutions only
      checking_priority,     // priority for checking feasibility. <0 means integral solutions only
      -1,     // frequency for separating cuts; 0 = only at root node
      0,     // frequency for propagating domains; 0 = only preprocessing propagation
      0,      // frequency for using all instead of only the useful constraints in separation, propagation and enforcement; -1 = no eager evaluations, 0 = first only
      -1,     // maximal number of presolving rounds the constraint handler participates in
      false,   // should separation method be delayed, if other separators found cuts?
      false,   // should propagation method be delayed, if other propagators found reductions?
      false,   // should the constraint handler be skipped, if no constraints are available?
      SCIP_PROPTIMING_AFTERLPNODE, // positions in the node solving loop where propagation method of constraint handlers should be executed
      SCIP_PRESOLTIMING_MEDIUM     // timing mask of the constraint handler's presolving method
    ),
    m_solver(solver),
    m_constr_hdlr(constr_hdlr) {
  }

  struct CurrentStateHandlerGuard {
    CurrentStateHandlerGuard(ScipSolver& solver, SCIP_SOL* p_sol) : m_solver(solver) {
      m_solver.p_current_state_handler = std::make_unique<detail::ScipCurrentStateHandle>(m_solver, p_sol);
    }
    ~CurrentStateHandlerGuard() {
      m_solver.p_current_state_handler.reset();
    }
    ScipSolver& m_solver;
  };

  SCIP_DECL_CONSCHECK(scip_check)
  {
    CurrentStateHandlerGuard guard(m_solver, sol);
    if (m_constr_hdlr.is_feasible())
      *result = SCIP_FEASIBLE;
    else
      *result = SCIP_INFEASIBLE;
    return SCIP_OKAY;
  }

  SCIP_DECL_CONSTRANS(scip_trans)
  {
    // FIXME: not sure yet if this is needed for anything
    return SCIP_OKAY;
  }

  SCIP_DECL_CONSENFOLP(scip_enfolp)
  {
    CurrentStateHandlerGuard guard(m_solver, nullptr);
    if (m_constr_hdlr.add())
      *result = SCIP_CONSADDED;
    else
      *result = SCIP_FEASIBLE;
    return SCIP_OKAY;
  }

  SCIP_DECL_CONSENFOPS(scip_enfops)
  {
    CurrentStateHandlerGuard guard(m_solver, nullptr);
    if (m_constr_hdlr.is_feasible())
      *result = SCIP_FEASIBLE;
    else
      *result = SCIP_INFEASIBLE;
    return SCIP_OKAY;    
  }

  SCIP_DECL_CONSLOCK(scip_lock)
  {
    CurrentStateHandlerGuard guard(m_solver, nullptr);
    for (auto const& v: m_constr_hdlr.depends())
    {
      auto p_var = static_cast<ScipVar const&>(*v.p_impl).p_var;
      SCIP_CALL_EXC(SCIPaddVarLocks(scip, p_var, nlocksneg + nlockspos, nlocksneg + nlockspos));
    }
    return SCIP_OKAY;
  }

  ScipSolver& m_solver;
  LazyConstrHandler m_constr_hdlr;
};

std::size_t ScipConstraintHandler::nr_instances = 0;

void ScipSolver::add_lazy_constr_handler(LazyConstrHandler const& constr_hdlr, bool at_integral_nodes_only)
{
  ScipConstraintHandler* p_scip_conshdlr;
  if (at_integral_nodes_only)
    p_scip_conshdlr = new ScipConstraintHandler(*this, constr_hdlr, -1, -1);
  else
    p_scip_conshdlr = new ScipConstraintHandler(*this, constr_hdlr, 1, 1);

  SCIP_CALL_EXC(SCIPincludeObjConshdlr(p_env, p_scip_conshdlr, true));
}

std::string ScipSolver::backend_info()
{
  return fmt::format(
    "SCIP {}",
    SCIP_VERSION/100.0
  );
}

bool ScipSolver::is_available()
{
  // If scip is compiled then it is also available.
  return true;
}

}  // namespace miplib

#pragma GCC diagnostic pop
