
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"


#include <miplib/gurobi/solver.hpp>
#include <miplib/var.hpp>
#include <miplib/gurobi/var.hpp>
#include <miplib/gurobi/constr.hpp>

#include <fmt/ostream.h>

namespace miplib {

GurobiSolver::GurobiSolver(): model(env), pending_update(false) {}

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
  }
  else
  if (e.is_quadratic()) {
    GRBQuadExpr grb_expr = as_grb_quad_expr(e);    
    model.setObjective(grb_expr, grb_sense);
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
  }
  else if (e.is_quadratic())
  {
    GRBQuadExpr lhs_expr = as_grb_quad_expr(e);

    char sense = (type == Constr::LessEqual) ? GRB_LESS_EQUAL : GRB_EQUAL;

    static_cast<GurobiQuadConstr const&>(*constr.p_impl).m_constr
      = model.addQConstr(lhs_expr, sense, 0, name.value_or(""));
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

double GurobiSolver::get_int_feasibility_tolerance() const
{
  return model.get(GRB_DoubleParam_IntFeasTol);
}

double GurobiSolver::get_feasibility_tolerance() const
{
  return model.get(GRB_DoubleParam_FeasibilityTol);
}

std::pair<Solver::Result, bool> GurobiSolver::solve()
{
  auto nr_sols_before = model.get(GRB_IntAttr_SolCount);
  model.optimize();
  bool has_solution = model.get(GRB_IntAttr_SolCount) > nr_sols_before;

  pending_update = false;

  auto r = model.get(GRB_IntAttr_Status);
  switch (r)
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

namespace detail {

GurobiCurrentStateHandle::GurobiCurrentStateHandle(LazyConstrHandler const& constr_hdlr) :
  m_constr_hdlr(constr_hdlr), m_active(false)
  {}

double GurobiCurrentStateHandle::value(IVar const& var) const
{ 
  GRBVar grb_var = static_cast<GurobiVar const&>(var).m_var;
  if (where == GRB_CB_MIPSOL or where == GRB_CB_MULTIOBJ)
    return const_cast<GurobiCurrentStateHandle*>(this)->getSolution(grb_var);
  else
  if (where == GRB_CB_MIPNODE)
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
  if (where != GRB_CB_MIPSOL)
    return;

  m_active = true;
  try {
    m_constr_hdlr.add();
  }
  catch (...) {
    m_active = false;
    throw;
  }
  m_active = false;
}

} // namespace detail

void GurobiSolver::set_lazy_constr_handler(LazyConstrHandler const& constr_hdlr)
{
  if (p_callback)
    throw std::logic_error("LazyConstrHandler already set.");
  model.set(GRB_IntParam_LazyConstraints, 1);
  p_callback = std::make_unique<detail::GurobiCurrentStateHandle>(constr_hdlr);
  model.setCallback(p_callback.get());
}

}  // namespace miplib

#pragma GCC diagnostic pop
