#include "var.hpp"
#include "solver.hpp"

#include <sstream>
#include <string>

namespace miplib {


Var::Var(
  Solver const& solver,
  Var::Type const& type,
  std::optional<double> const& lb,
  std::optional<double> const& ub,
  std::optional<std::string> const& name
):
  p_impl(solver.p_impl->create_var(solver, type, lb, ub, name))
{}


Var::Var(Solver const& solver, Var::Type const& type, std::string const& name):
  Var(solver, type, std::nullopt, std::nullopt, name)
{}


bool Var::is_same(Var const& v1) const
{
  return p_impl == v1.p_impl;
}


bool Var::is_lex_less(Var const& v1) const
{
  return p_impl < v1.p_impl;
}


std::string Var::id() const
{
  auto const& impl = *p_impl;
  if (impl.name().has_value())
    return impl.name().value();

  std::stringstream s;
  s << p_impl.get();
  return s.str();
}


double Var::value() const
{
  return p_impl->value();
}


Var::Type Var::type() const
{
  return p_impl->type();
}


Solver const& Var::solver() const
{
  return p_impl->solver();
}


std::ostream& operator<<(std::ostream& os, Var const& v)
{
  os << v.id();
  return os;
}

}  // namespace miplib
