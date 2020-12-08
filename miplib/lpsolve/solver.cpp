#include <miplib/lpsolve/solver.hpp>
#include <miplib/lpsolve/util.hpp>

#include <miplib/lpsolve/var.hpp>
#include <miplib/lpsolve/constr.hpp>

#include <fmt/ostream.h>


namespace miplib {


LpsolveSolver::LpsolveSolver(): p_lprec(make_lp(0, 0))
{}


LpsolveSolver::~LpsolveSolver() noexcept(false)
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

void LpsolveSolver::add(IndicatorConstr const&)
{
  throw std::logic_error("Lpsolve does not support indicator constraints.");
}


Solver::Result LpsolveSolver::solve()
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

  // not sure yet if this can be called for any return status
  store_solution();

  switch (status)
  {
    case 0: // optimal
    case 9: // optimal (solved by presolve)
      return Solver::Result::Optimal;

    case 1: // Suboptimal
      return Solver::Result::Interrupted;

    case 2:
      return Solver::Result::Infeasible;

    case 3:
      return Solver::Result::Unbounded;


    case -2: // Out of memory
    case 6: // User aborted
    case 7: // Timeout
      return Solver::Result::Interrupted;

    case 4: // Degenerate
    case 5: // Numerical failure
    case 25: // Accuracy error
      return Solver::Result::Error;

    default:
      return Solver::Result::Other;
  }
}

void LpsolveSolver::set_non_convex_policy(Solver::NonConvexPolicy /*policy*/)
{
  // Not supported by lpsolve.
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


}  // namespace miplib