#include <miplib/lpsolve/solver.hpp>
#include <miplib/lpsolve/util.hpp>

#include <miplib/lpsolve/var.hpp>
#include <miplib/lpsolve/constr.hpp>

#include <fmt/ostream.h>


namespace miplib {


LpsolveSolver::LpsolveSolver(bool verbose): p_lprec(make_lp(0, 0))
{
  set_verbose(verbose);
}


LpsolveSolver::~LpsolveSolver()
{
  delete_lp(p_lprec);
}


std::shared_ptr<detail::IVar> LpsolveSolver::create_var(
  Solver const& solver,
  Var::Type const& type,
  std::optional<double> const& lb,
  std::optional<double> const& ub,
  std::optional<std::string> const& name
)
{
  return std::make_shared<LpsolveVar>(solver, type, lb, ub, name);
}

std::shared_ptr<detail::IConstr> LpsolveSolver::create_constr(
  Constr::Type const& type, Expr const& e, std::optional<std::string> const& name
)
{
  return std::make_shared<LpsolveConstr>(e, type, name);
}

std::shared_ptr<detail::IIndicatorConstr> LpsolveSolver::create_indicator_constr(
  Constr const& implicant,
  Constr const& implicand,
  std::optional<std::string> const& name
)
{
  return detail::create_reformulatable_indicator_constr(implicant, implicand, name);
}

std::vector<int> LpsolveSolver::get_col_idxs(std::vector<Var> const& vars)
{
  std::vector<int> r;
  std::transform(
    vars.begin(), vars.end(),
    std::back_inserter(r),
    [](Var const& v) {
      return static_cast<LpsolveVar const&>(*v.p_impl).cur_col_idx();
    }
  );
  return r;
}


void LpsolveSolver::set_objective(Solver::Sense const& sense, Expr const& e)
{
  if (e.is_linear())
  {
    auto col_idxs = get_col_idxs(e.linear_vars());  
    auto coeffs = e.linear_coeffs();

    bool r = set_obj_fnex(p_lprec, col_idxs.size(), coeffs.data(), col_idxs.data());
    if (!r)
      throw std::logic_error("Lpsolve error setting objective.");
  }
  else
  if (e.is_quadratic())
  {
    throw std::logic_error("Lpsolve does not support quadratic objective functions.");
  }
  else
    assert(false);

  set_sense(p_lprec, sense == Solver::Sense::Maximize);
}


double LpsolveSolver::get_objective_value() const
{
  return get_objective(p_lprec);
}

Solver::Sense LpsolveSolver::get_objective_sense() const
{
  throw std::logic_error("Lpsolve does not support retrieving current objective sense.");
}

void LpsolveSolver::add(Constr const& constr)
{
  auto const& constr_impl = static_cast<LpsolveConstr const&>(*constr.p_impl);

  if (constr_impl.m_orig_row_idx >= 0)
  {
    throw std::logic_error("Attempt to post the same constraint twice.");
  }

  Expr const& e = constr.expr();
  if (e.is_quadratic())
    throw std::logic_error("Lpsolve does not support quadratic constraints.");

  auto col_idxs = get_col_idxs(e.linear_vars());
  auto coeffs = e.linear_coeffs();

  int constr_type;
  if (constr.type() == Constr::Type::LessEqual)
    constr_type = 1;
  else
  {
    assert(constr.type() == Constr::Type::Equal);
    constr_type = 3;
  }

  bool r = add_constraintex(
    p_lprec, col_idxs.size(), coeffs.data(), col_idxs.data(), constr_type, -e.constant()
  );
  if (!r)
    throw std::logic_error("Lpsolve error adding constraint.");

  constr_impl.m_orig_row_idx = get_Nrows(p_lprec);
}

bool LpsolveSolver::supports_indicator_constraint(IndicatorConstr const&) const
{
  return false;
}

void LpsolveSolver::add(IndicatorConstr const&)
{
  throw std::logic_error("Lpsolve does not support indicator constraints.");
}

void LpsolveSolver::remove(Constr const&)
{
  // TODO: removing a constraint changes other constraints idxs, 
  // meaning we need to keep track of them and update them accordingly.
  throw std::logic_error("Not implemented yet.");
}

std::pair<Solver::Result, bool> LpsolveSolver::solve()
{
  // presolve
  int PRESOLVE_TRY_ALL_TRICKS = std::numeric_limits<int>::max();
  set_presolve(p_lprec, PRESOLVE_TRY_ALL_TRICKS, get_presolveloops(p_lprec));

  int status = ::solve(p_lprec);

  m_last_solution.clear();

  // store solution
  auto store_solution = [&]() {
    int nr_orig_columns = get_Norig_columns(p_lprec);
    int nr_orig_rows = get_Norig_rows(p_lprec);
    for(int i = 1; i <= nr_orig_columns; i++)
      m_last_solution.push_back(get_var_primalresult(p_lprec, nr_orig_rows + i));
  };

  if (status == 0 or status == 9 or status == 1)
    store_solution();

  switch (status)
  {
    case 0: // optimal
    case 9: // optimal (solved by presolve)
      return {Solver::Result::Optimal, true};

    case 1: // Suboptimal
      return {Solver::Result::Interrupted, true};

    case 2:
      return {Solver::Result::Infeasible, false};

    case 3:
      return {Solver::Result::Unbounded, false};


    case -2: // Out of memory
    case 6: // User aborted
    case 7: // Timeout
      return {Solver::Result::Interrupted, false};

    case 4: // Degenerate
    case 5: // Numerical failure
    case 25: // Accuracy error
      return {Solver::Result::Error, false};

    default:
      return {Solver::Result::Other, false};
  }
}

void LpsolveSolver::set_non_convex_policy(Solver::NonConvexPolicy /*policy*/)
{
  // Not supported by lpsolve.
}

void LpsolveSolver::set_int_feasibility_tolerance(double value)
{
  set_epsint(p_lprec, value);
}

void LpsolveSolver::set_feasibility_tolerance(double value)
{
  set_epsb(p_lprec, value);
}

void LpsolveSolver::set_epsilon(double value)
{
  set_epsel(p_lprec, value);
}

void LpsolveSolver::set_nr_threads(std::size_t nr_threads)
{
  if (nr_threads > 1)
    throw std::logic_error("LPSolver does not support concurrent solving");
}

double LpsolveSolver::get_int_feasibility_tolerance() const
{
  return get_epsint(p_lprec);
}

double LpsolveSolver::get_feasibility_tolerance() const
{
 return get_epsb(p_lprec);
}

double LpsolveSolver::get_epsilon() const
{
 return get_epsel(p_lprec);
}

void LpsolveSolver::set_verbose(bool value)
{
  if (value)
    ::set_verbose(p_lprec, 4);
  else
    ::set_verbose(p_lprec, 1);
}


double LpsolveSolver::infinity() const
{
  return get_infinite(p_lprec);
}

void LpsolveSolver::set_time_limit(double secs)
{
  set_timeout(p_lprec, (long) std::ceil(secs));
}

void LpsolveSolver::dump(std::string const& filename) const
{
  std::string ext = filename.substr(filename.size()-3);
  if (ext == "lp" or ext == "LP")
    write_lp(p_lprec, const_cast<char*>(filename.c_str()));
  else
  if (ext == "mps" or ext == "MPS")
    write_mps(p_lprec, const_cast<char*>(filename.c_str()));
  else
    throw std::logic_error(
      fmt::format("Dumping lpsolve models to {} is not is not supported.", ext)
    );
}

void LpsolveSolver::set_warm_start(PartialSolution const& /*partial_solution*/)
{
  throw std::logic_error("Lpsolve does not support warm starts.");
}

void LpsolveSolver::set_reoptimizing(bool)
{
  // Lpsolve does not require explicitely enabling/disabling reoptimization.
}

void LpsolveSolver::setup_reoptimization()
{
  // Lpsolve does not require doing anything explicit before reoptimizing.
}

std::string LpsolveSolver::backend_info()
{
  int major, minor, release, build;
  lp_solve_version(&major, &minor, &release, &build);
  return fmt::format(
    "Lpsolve {}.{}.{}.{}",
    major,
    minor,
    release,
    build
  );
}

bool LpsolveSolver::is_available()
{
  // If lpsolve is compiled then it is also available.
  return true;
}

}  // namespace miplib