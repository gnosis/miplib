
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"


#include <miplib/gurobi/solver.hpp>
#include <miplib/var.hpp>
#include <miplib/gurobi/var.hpp>
#include <miplib/gurobi/constr.hpp>

#include <fmt/ostream.h>
#include <spdlog/spdlog.h>

namespace miplib {

GurobiSolver::GurobiSolver() :
  model(env),
  pending_update(false)
  {}

void GurobiSolver::set_pending_update() const
{
  pending_update = true;
}

void GurobiSolver::update_if_pending() const
{
  if (!pending_update)
    return;
  model.update();
  pending_update = false;
}

std::shared_ptr<detail::IVar> GurobiSolver::create_var(
  Solver const& solver,
  Var::Type const& type,
  std::optional<double> const& lb,
  std::optional<double> const& ub,
  std::optional<std::string> const& name
)
{
  char grb_var_type;
  double grb_lb, grb_ub;

  if (type == Var::Type::Continuous)
  {
    grb_var_type = GRB_CONTINUOUS;
    grb_lb = lb.value_or(-GRB_INFINITY);
    grb_ub = ub.value_or(GRB_INFINITY);
  }
  else if (type == Var::Type::Binary)
  {
    grb_var_type = GRB_BINARY;
    if (lb.value_or(0) != 0 or ub.value_or(1) != 1)
    {
      throw std::logic_error("Binary variables bounds must be 0..1.");
    }
    grb_lb = 0;
    grb_ub = 1;
  }
  else if (type == Var::Type::Integer)
  {
    grb_var_type = GRB_INTEGER;
    grb_lb = lb.value_or(-GRB_INFINITY);
    grb_ub = ub.value_or(GRB_INFINITY);
  }
  else
  {
    throw std::logic_error("Gurobi does not support this variable type");
  }

  GRBVar grb_var = model.addVar(grb_lb, grb_ub, 0.0, grb_var_type, name.value_or(""));
  return std::make_shared<GurobiVar>(solver, grb_var);
}

static GRBLinExpr as_grb_lin_expr(Expr const& e)
{
  assert(e.is_linear());
  GRBLinExpr ge = e.constant();

  auto coeffs = e.linear_coeffs();
  auto vars = e.linear_vars();

  assert(coeffs.size() == vars.size());
  assert(!coeffs.empty());

  std::vector<GRBVar> grb_vars;
  std::transform(
    vars.begin(), vars.end(), std::back_inserter(grb_vars), [](auto const& v) {
      return static_cast<GurobiVar const&>(*v.p_impl).m_var;
    });
  ge.addTerms(coeffs.data(), grb_vars.data(), coeffs.size());
  return ge;
}

static GRBQuadExpr as_grb_quad_expr(Expr const& e)
{
  assert(e.is_quadratic());
  GRBQuadExpr ge = e.constant();

  // add linear terms
  if (!e.linear_coeffs().empty())
  {
    auto coeffs = e.linear_coeffs();
    auto vars = e.linear_vars();

    assert(coeffs.size() == vars.size());
    assert(!coeffs.empty());

    std::vector<GRBVar> grb_vars;
    std::transform(
      vars.begin(), vars.end(), std::back_inserter(grb_vars), [](auto const& v) {
        return static_cast<GurobiVar const&>(*v.p_impl).m_var;
      });
    ge.addTerms(coeffs.data(), grb_vars.data(), coeffs.size());
  }

  // add quad terms
  auto coeffs = e.quad_coeffs();
  auto vars_1 = e.quad_vars_1();
  auto vars_2 = e.quad_vars_2();

  assert(coeffs.size() == vars_1.size());
  assert(coeffs.size() == vars_2.size());
  assert(!coeffs.empty());

  std::vector<GRBVar> grb_vars_1;
  std::transform(
    vars_1.begin(), vars_1.end(), std::back_inserter(grb_vars_1), [](auto const& v) {
      return static_cast<GurobiVar const&>(*v.p_impl).m_var;
    });

  std::vector<GRBVar> grb_vars_2;
  std::transform(
    vars_2.begin(), vars_2.end(), std::back_inserter(grb_vars_2), [](auto const& v) {
      return static_cast<GurobiVar const&>(*v.p_impl).m_var;
    });

  ge.addTerms(coeffs.data(), grb_vars_1.data(), grb_vars_2.data(), coeffs.size());

  return ge;
}

std::shared_ptr<detail::IConstr> GurobiSolver::create_constr(
  Constr::Type const& type, Expr const& e, std::optional<std::string> const& name
)
{
  if (e.is_linear())
  {
    return std::make_shared<GurobiLinConstr>(e, type, name);
  }
  else if (e.is_quadratic())
  {
    return std::make_shared<GurobiQuadConstr>(e, type, name);
  }
  throw std::logic_error(
    fmt::format("Gurobi does not support constraint involving expression {}.", e));
}

std::shared_ptr<detail::IIndicatorConstr> GurobiSolver::create_indicator_constr(
  Constr const& implicant,
  Constr const& implicand,
  std::optional<std::string> const& name)
{
  return std::make_shared<GurobiIndicatorConstr>(implicant, implicand, name);
}

void GurobiSolver::set_objective(Solver::Sense const& sense, Expr const& e)
{
  int grb_sense = sense == Solver::Sense::Minimize ? GRB_MINIMIZE : GRB_MAXIMIZE;
  if (e.is_linear())
  {
    GRBLinExpr grb_expr = as_grb_lin_expr(e);
    model.setObjective(grb_expr, grb_sense);
    model_has_changed_since_last_solve = true;
  }
  else
  if (e.is_quadratic()) {
    GRBQuadExpr grb_expr = as_grb_quad_expr(e);    
    model.setObjective(grb_expr, grb_sense);
    model_has_changed_since_last_solve = true;
  }
}

double GurobiSolver::get_objective_value() const
{
  return model.get(GRB_DoubleAttr_ObjVal);
}

Solver::Sense GurobiSolver::get_objective_sense() const
{
  auto grb_sense = model.get(GRB_IntAttr_ModelSense);
  if (grb_sense == GRB_MINIMIZE)
    return Solver::Sense::Minimize;
  else
  {
    assert(grb_sense == GRB_MAXIMIZE);
    return Solver::Sense::Maximize;
  }
}

void GurobiSolver::add(Constr const& constr)
{
  auto const& e = constr.expr();
  auto const& type = constr.type();
  auto const& name = constr.name();

  if (is_in_callback())
  {
    p_callback->add_lazy(constr);
    return;
  }

  if (e.is_linear())
  {
    GRBLinExpr lhs_expr = as_grb_lin_expr(e);

    char sense = (type == Constr::LessEqual) ? GRB_LESS_EQUAL : GRB_EQUAL;

    static_cast<GurobiLinConstr const&>(*constr.p_impl).m_constr
      = model.addConstr(lhs_expr, sense, 0, name.value_or(""));
    
    model_has_changed_since_last_solve = true;
  }
  else if (e.is_quadratic())
  {
    GRBQuadExpr lhs_expr = as_grb_quad_expr(e);

    char sense = (type == Constr::LessEqual) ? GRB_LESS_EQUAL : GRB_EQUAL;

    static_cast<GurobiQuadConstr const&>(*constr.p_impl).m_constr
      = model.addQConstr(lhs_expr, sense, 0, name.value_or(""));

    model_has_changed_since_last_solve = true;
  }
  else
  {
    throw std::logic_error(fmt::format("Gurobi does not support constraint {}.", constr));
  }
}

bool GurobiSolver::supports_indicator_constraint(IndicatorConstr const& constr) const
{
  auto const& implicant = constr.implicant();
  auto const& implicand = constr.implicand();

  // Gurobi indicator constraints require an equation implicant.
  if (implicant.type() != Constr::Equal)
    return false;

  // Gurobi indicator constraints require a linear implicant.
  if (!implicant.expr().is_linear())
    return false;

  auto implicant_linear_vars = implicant.expr().linear_vars();
  auto implicant_linear_coeffs = implicant.expr().linear_coeffs();

  // Gurobi indicator constraints require single variable implicant.
  if (implicant_linear_vars.size() != 1)
    return false;

  double c = -implicant_linear_coeffs[0] * implicant.expr().constant();
  // Gurobi indicator constraints require a 'v = [0|1]' implicant.
  if (c != 0 and c != 1)
    return false;

  // Gurobi indicator constraints require a linear implicand.
  if (!implicand.expr().is_linear())
    return false;

  return true;
}

void GurobiSolver::add(IndicatorConstr const& constr)
{
  if (!supports_indicator_constraint(constr))
    throw std::logic_error(
      "Gurobi doesn't support this indicator constraint. Try ctr.reformulation()."
    );

  if (is_in_callback())
    throw std::logic_error(
      "Gurobi doesn't support adding indicator constraints during solving. Try ctr.reformulation()."
    );

  auto const& implicant = constr.implicant();
  auto const& implicand = constr.implicand();
  auto const& name = constr.name();

  auto implicant_linear_vars = implicant.expr().linear_vars();
  auto implicant_linear_coeffs = implicant.expr().linear_coeffs();

  assert(implicant.type() == Constr::Equal);
  assert(implicant.expr().is_linear());

  assert(implicant_linear_vars.size() == 1);
  double c = -implicant_linear_coeffs[0] * implicant.expr().constant();
  assert(c == 0 or c == 1);

  int bin_val = (int) c;
  GRBVar bin_var = static_cast<GurobiVar const&>(*implicant_linear_vars[0].p_impl).m_var;

  assert(implicand.expr().is_linear());

  GRBLinExpr implicand_expr = as_grb_lin_expr(implicand.expr());

  char sense = (implicand.type() == Constr::LessEqual) ? GRB_LESS_EQUAL : GRB_EQUAL;

  static_cast<GurobiIndicatorConstr const&>(*constr.p_impl).m_constr
    = model.addGenConstrIndicator(
      bin_var, bin_val, implicand_expr, sense, 0, name.value_or(""));

  model_has_changed_since_last_solve = true;
}

void GurobiSolver::remove(Constr const& constr)
{
  if (std::dynamic_pointer_cast<GurobiLinConstr>(constr.p_impl))
  {
    auto const& c = static_cast<GurobiLinConstr const&>(*constr.p_impl);
    model.remove(c.m_constr.value());
    model_has_changed_since_last_solve = true;
  }
  else
  if (std::dynamic_pointer_cast<GurobiQuadConstr>(constr.p_impl))
  {
    auto const& c = static_cast<GurobiQuadConstr const&>(*constr.p_impl);
    model.remove(c.m_constr.value());
    model_has_changed_since_last_solve = true;
  }
  else
    assert(false);
}

void GurobiSolver::set_non_convex_policy(Solver::NonConvexPolicy policy)
{
  switch (policy)
  {
    case Solver::NonConvexPolicy::Error:    
      model.set(GRB_IntParam_NonConvex, 0);
      break;
    case Solver::NonConvexPolicy::Linearize:
      model.set(GRB_IntParam_NonConvex, 1);
      break;
    case Solver::NonConvexPolicy::Branch:
      model.set(GRB_IntParam_NonConvex, 2);
      break;
    default:
      assert(false);
  }
}


void GurobiSolver::set_verbose(bool value)
{
  model.set(GRB_IntParam_OutputFlag, value);
}

void GurobiSolver::set_int_feasibility_tolerance(double value)
{
  model.set(GRB_DoubleParam_IntFeasTol, value);
}

void GurobiSolver::set_feasibility_tolerance(double value)
{
  model.set(GRB_DoubleParam_FeasibilityTol, value);
}

void GurobiSolver::set_epsilon(double /*value*/)
{
  // it seems it is not possible to set this value in gurobi
}

void GurobiSolver::set_nr_threads(std::size_t nr_threads)
{
  model.set(GRB_IntParam_Threads, nr_threads);
}

double GurobiSolver::get_int_feasibility_tolerance() const
{
  return model.get(GRB_DoubleParam_IntFeasTol);
}

double GurobiSolver::get_feasibility_tolerance() const
{
  return model.get(GRB_DoubleParam_FeasibilityTol);
}

double GurobiSolver::get_epsilon() const
{
  // it seems it is not possible to retrieve this value in gurobi
  // returning this as a proxy.
  return get_feasibility_tolerance();
}

static std::pair<Solver::Result, bool> grb_status_to_solver_result(int grb_status, bool has_solution)
{
  switch (grb_status)
  {
    case GRB_OPTIMAL:
      return {Solver::Result::Optimal, true};

    case GRB_INFEASIBLE:
      return {Solver::Result::Infeasible, false};

    case GRB_INF_OR_UNBD:
      return {Solver::Result::InfeasibleOrUnbounded, false};

    case GRB_UNBOUNDED:
      return {Solver::Result::Unbounded, false};

    case GRB_CUTOFF:
    case GRB_ITERATION_LIMIT:
    case GRB_NODE_LIMIT:
    case GRB_TIME_LIMIT:
    case GRB_SOLUTION_LIMIT:
    case GRB_INTERRUPTED:
    case GRB_USER_OBJ_LIMIT:
    case GRB_SUBOPTIMAL:
      return {Solver::Result::Interrupted, has_solution};

    case GRB_NUMERIC:
      return {Solver::Result::Error, has_solution};

    default:
      return {Solver::Result::Other, has_solution};
  }
}

std::pair<Solver::Result, bool> GurobiSolver::solve()
{
  if (!model_has_changed_since_last_solve)
  {
    spdlog::warn("Will not resolve a model that has not changed .");
    auto grb_status = model.get(GRB_IntAttr_Status);
    bool has_solution = model.get(GRB_IntAttr_SolCount) > 0; 
    return grb_status_to_solver_result(grb_status, has_solution);
  }

  model.optimize();
  bool has_solution = model.get(GRB_IntAttr_SolCount) > 0;

  pending_update = false;
  model_has_changed_since_last_solve = false;

  auto grb_status = model.get(GRB_IntAttr_Status);
  return grb_status_to_solver_result(grb_status, has_solution);
}

double GurobiSolver::infinity() const
{
  return GRB_INFINITY;
}

void GurobiSolver::dump(std::string const& filename) const
{
  model.write(filename);
}

void GurobiSolver::set_time_limit(double secs)
{
  model.set(GRB_DoubleParam_TimeLimit, secs);
}

bool GurobiSolver::is_in_callback() const
{
  return p_callback and p_callback->is_active();
}

void GurobiSolver::set_warm_start(PartialSolution const& partial_solution)
{
  if (is_in_callback())
    throw std::logic_error("Cannot add warm start from callback.");

  for (auto const& [var, val]: partial_solution)
  {
    auto& m_var = static_cast<GurobiVar&>(*var.p_impl);
    m_var.set_start_value(val);
  }
}

void GurobiSolver::set_reoptimizing(bool)
{
  // GurobiSolver does not require explicitely enabling/disabling reoptimization.
}

void GurobiSolver::setup_reoptimization()
{
  // Gurobi does not require doing anything explicit before reoptimizing.
}

namespace detail {

GurobiCurrentStateHandle::GurobiCurrentStateHandle() : m_active(false)
{}

void GurobiCurrentStateHandle::add_constr_handler(LazyConstrHandler const& constr_hdlr, bool integral_only)
{
  if (integral_only)
    m_integral_only_constr_hdlrs.push_back(constr_hdlr);
  else
    m_constr_hdlrs.push_back(constr_hdlr);
}

double GurobiCurrentStateHandle::value(IVar const& var) const
{ 
  GRBVar grb_var = static_cast<GurobiVar const&>(var).m_var;
  if (where == GRB_CB_MIPSOL or where == GRB_CB_MULTIOBJ)
    return const_cast<GurobiCurrentStateHandle*>(this)->getSolution(grb_var);
  else
  if (
    where == GRB_CB_MIPNODE and 
    const_cast<GurobiCurrentStateHandle*>(this)->getIntInfo(GRB_CB_MIPNODE_STATUS) == GRB_OPTIMAL
  )
    return const_cast<GurobiCurrentStateHandle*>(this)->getNodeRel(grb_var);

  throw std::logic_error("Failure to obtain variable value from current node.");
}

void GurobiCurrentStateHandle::add_lazy(Constr const& constr)
{
  auto const& e = constr.expr();
  auto const& type = constr.type();

  if (e.is_linear())
  {
    GRBLinExpr lhs_expr = as_grb_lin_expr(e);
    char sense = (type == Constr::LessEqual) ? GRB_LESS_EQUAL : GRB_EQUAL;
    addLazy(lhs_expr, sense, 0);
  }
  else
    throw std::logic_error("Gurobi supports lazy linear constraints only.");
}

void GurobiCurrentStateHandle::callback()
{
  m_active = true;
  try 
  {
    // if we are in an integral or non integral node
    if (
      where == GRB_CB_MIPSOL or
      (where == GRB_CB_MIPNODE and getIntInfo(GRB_CB_MIPNODE_STATUS) == GRB_OPTIMAL)
    )
    {
      for (auto& h : m_constr_hdlrs)
        h.add();
    }

    // if we are in an integral node
    if (where == GRB_CB_MIPSOL)
    {
      for (auto& h: m_integral_only_constr_hdlrs)
        h.add();
    }
  }
  catch (...)
  {
    m_active = false;
    throw;
  }
  m_active = false;
}

} // namespace detail

void GurobiSolver::add_lazy_constr_handler(LazyConstrHandler const& constr_hdlr, bool at_integral_only)
{
  if (!p_callback)
  {
    model.set(GRB_IntParam_LazyConstraints, 1);
    p_callback = std::make_unique<detail::GurobiCurrentStateHandle>();
    model.setCallback(p_callback.get());
  }
  p_callback->add_constr_handler(constr_hdlr, at_integral_only);
}

std::string GurobiSolver::backend_info()
{
  return fmt::format(
    "Gurobi {}.{}.{}",
    GRB_VERSION_MAJOR,
    GRB_VERSION_MINOR,
    GRB_VERSION_TECHNICAL
  );
}

bool GurobiSolver::is_available()
{
  auto const APITYPE_CPP = 1;
  GRBenv*  env = nullptr;
  int  error = GRBloadenvadv(
    &env, NULL, APITYPE_CPP, GRB_VERSION_MAJOR,
    GRB_VERSION_MINOR, GRB_VERSION_TECHNICAL,
    NULL, NULL, NULL, NULL, -9999999, -1,
    NULL, NULL, NULL, NULL, NULL
  );
  if (env != nullptr)
    GRBfreeenv(env);
  if (error)
    return false;
  return true;
}

}  // namespace miplib

#pragma GCC diagnostic pop
