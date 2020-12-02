#include <miplib/lpsolve/var.hpp>

#include <miplib/lpsolve/util.hpp>

namespace miplib {

LpsolveVar::LpsolveVar(
  Solver const& solver,
  Var::Type const& type,
  std::optional<double> const& lb,
  std::optional<double> const& ub,
  std::optional<std::string> const& name
):
  m_solver(solver)
{
  auto p_lprec = static_cast<LpsolveSolver const&>(*m_solver.p_impl).p_lprec;

  // create var
  bool success = add_columnex(p_lprec, 0, NULL, NULL);
  if (!success)
    throw std::logic_error("lpsolve error creating variable.");

  // arrays are 1 based (!)
  m_orig_col_idx = get_Ncolumns(p_lprec);

  // set type
  if (type == Var::Type::Binary)
    set_binary(p_lprec, m_orig_col_idx, 1);
  else
  if (type == Var::Type::Integer)
    set_int(p_lprec, m_orig_col_idx, 1);
  else
  if (type != Var::Type::Continuous)
    throw std::logic_error("lpsolve does not support this variable type");

  // set bounds
  if (type != Var::Type::Binary)
  {
    auto inf = get_infinite(p_lprec);
    success = set_bounds(
      p_lprec,
      m_orig_col_idx,
      lb ? lb.value() : -inf,
      ub ? ub.value() : inf
    );
    if (!success)
      throw std::logic_error("lpsolve error setting variable bounds.");
  }

  // set name
  if (name)
  {
    char* cname = const_cast<char*>(name->c_str());
    success = set_col_name(p_lprec, m_orig_col_idx, cname);
    if (!success)
      throw std::logic_error("lpsolve error naming variable.");
  }
}


int LpsolveVar::cur_col_idx() const
{
  auto p_lprec = static_cast<LpsolveSolver const&>(*m_solver.p_impl).p_lprec;
  return get_cur_col_index(p_lprec, m_orig_col_idx);
}


double LpsolveVar::value() const
{
  auto const& lpsolve_solver = static_cast<LpsolveSolver const&>(*m_solver.p_impl);
  auto const& last_solution = lpsolve_solver.m_last_solution;

  if (last_solution.size() < m_orig_col_idx)
  {
    throw std::logic_error(
      "Attempt to access value of variable before a solution was found."
    );
  }
  return last_solution[m_orig_col_idx - 1];
}

Var::Type LpsolveVar::type() const
{
  auto p_lprec = static_cast<LpsolveSolver const&>(*m_solver.p_impl).p_lprec;
  int col_idx = cur_col_idx();

  if (is_binary(p_lprec, col_idx))
    return Var::Type::Binary;
  else
  if (is_int(p_lprec, col_idx))
    return Var::Type::Integer;
  else
    return Var::Type::Continuous;
}

std::optional<std::string> LpsolveVar::name() const
{
  auto p_lprec = static_cast<LpsolveSolver const&>(*m_solver.p_impl).p_lprec;
  return std::string(get_origcol_name(p_lprec, m_orig_col_idx));
}

}  // namespace miplib
