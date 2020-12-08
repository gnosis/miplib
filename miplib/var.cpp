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

// Returns name stored in backend.
std::optional<std::string> Var::name() const
{
  return p_impl->name();
}

// Sets name of variable in backend.
void Var::set_name(std::string const& new_name)
{
  p_impl->set_name(new_name);
}

// Returns name stored in backend if not null, 
// otherwise generates a unique name.
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


double Var::lb() const
{
  return p_impl->lb();
}

double Var::ub() const
{
  return p_impl->ub(); 
}

void Var::set_lb(double new_lb)
{
  p_impl->set_lb(new_lb);
}

void Var::set_ub(double new_ub)
{
  p_impl->set_ub(new_ub);
}

std::ostream& operator<<(std::ostream& os, Var const& v)
{
  os << v.id();
  return os;
}

}  // namespace miplib
