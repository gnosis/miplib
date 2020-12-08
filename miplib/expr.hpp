#pragma once

#include "var.hpp"
#include "util.hpp"

#include <memory>
#include <unordered_map>
#include <iostream>

namespace miplib {

namespace detail {

struct ExprImpl
{
  ExprImpl(double c = 0): m_constant(c) {}
  ExprImpl(Var const v): m_linear({{v, 1}}), m_constant(0) {}
  ExprImpl(ExprImpl const& e) = default;

  ExprImpl& operator+=(double c);
  ExprImpl& operator+=(Var v);
  ExprImpl& operator+=(ExprImpl const& e);
  ExprImpl& operator-=(Var v);
  ExprImpl& operator*=(double c);
  ExprImpl& operator*=(Var v);
  ExprImpl& operator*=(ExprImpl const& e);
  ExprImpl& operator/=(double c);

  Solver const& solver() const;

  std::unordered_map<Var, double> m_linear;
  std::unordered_map<std::pair<Var, Var>, double> m_quad;
  double m_constant;
};

std::ostream& operator<<(std::ostream& os, ExprImpl const& e);

}  // namespace detail

struct Expr
{
  Expr(double c = 0): p_impl(std::make_shared<detail::ExprImpl>(c)) {}
  Expr(Var const& v): p_impl(std::make_shared<detail::ExprImpl>(v)) {}

  Expr(Expr const& e): p_impl(e.p_impl) {}

  Expr copy() const;

  bool is_constant() const
  {
    return p_impl->m_linear.empty() and p_impl->m_quad.empty();
  }

  bool is_linear() const
  {
    return p_impl->m_quad.empty();
  }

  bool is_quadratic() const
  {
    return !p_impl->m_quad.empty();
  }

  bool must_be_binary() const;

  // constant term
  double constant() const
  {
    return p_impl->m_constant;
  }

  // coefficients of linear part
  std::vector<double> linear_coeffs() const;

  // variables of linear part
  std::vector<Var> linear_vars() const;

  // coefficients of quad part
  std::vector<double> quad_coeffs() const;

  // variables of quad part
  std::vector<Var> quad_vars_1() const;

  // variables of quad part
  std::vector<Var> quad_vars_2() const;

  Solver const& solver() const
  {
    return p_impl->solver();
  }

  Expr& operator+=(double c)
  {
    *p_impl += c;
    return *this;
  }

  Expr& operator+=(Var const& v)
  {
    *p_impl += v;
    return *this;
  }

  Expr& operator+=(Expr const& e)
  {
    *p_impl += *e.p_impl;
    return *this;
  }

  Expr& operator-=(Var const& v)
  {
    *p_impl -= v;
    return *this;
  }

  Expr& operator-=(Expr const& e)
  {
    *p_impl += *(-e).p_impl;
    return *this;
  }

  Expr& operator*=(double c)
  {
    *p_impl *= c;
    return *this;
  }

  Expr& operator*=(Var const& v)
  {
    *p_impl *= v;
    return *this;
  }

  Expr& operator*=(Expr const& e)
  {
    *p_impl *= *e.p_impl;
    return *this;
  }

  Expr& operator/=(double c)
  {
    *p_impl /= c;
    return *this;
  }

  Expr operator-() const;

  Expr operator+(double c) const;
  Expr operator+(Var const& v) const;
  Expr operator+(Expr const& e) const;

  Expr operator-(Var const& v) const;

  Expr operator-(Expr const& e) const
  {
    return *this + (-e);
  };

  Expr operator*(double c) const;
  Expr operator*(Var const& v) const;
  Expr operator*(Expr const& e) const;

  Expr operator/(double c) const
  {
    return *this * (1 / c);
  }

  Expr operator/(Expr const& e) const;

  double lb() const;
  double ub() const;
  
  std::vector<Var> vars() const;
  std::size_t arity() const;

  private:
  std::shared_ptr<detail::ExprImpl> p_impl;
  friend std::ostream& operator<<(std::ostream& os, Expr const& e);
};

inline Expr operator-(Var const& v)
{
  return -Expr(v);
}

inline Expr operator+(Var const& v, double c)
{
  return Expr(v) + c;
}

inline Expr operator+(double c, Var const& v)
{
  return Expr(v) + c;
}

inline Expr operator+(double c, Expr const& e)
{
  return e + c;
}

inline Expr operator+(Var const& v1, Var const& v2)
{
  return Expr(v1) + v2;
}

inline Expr operator+(Var const& v, Expr const& e)
{
  return Expr(v) + e;
}

inline Expr operator-(Var const& v, double c)
{
  return Expr(v) + (-c);
}

inline Expr operator-(double c, Var const& v)
{
  return c + (-v);
}

inline Expr operator-(double c, Expr const& e)
{
  return c + (-e);
}

inline Expr operator-(Var const& v1, Var const& v2)
{
  return Expr(v1) + (-v2);
}

inline Expr operator-(Var const& v, Expr const& e)
{
  return Expr(v) + (-e);
}

inline Expr operator*(Var const& v, double c)
{
  return Expr(v) * c;
}

inline Expr operator*(double c, Var const& v)
{
  return Expr(v) * c;
}

inline Expr operator*(double c, Expr const& e)
{
  return e * c;
}

inline Expr operator*(Var const& v1, Var const& v2)
{
  return Expr(v1) * v2;
}

inline Expr operator*(Var const& v, Expr const& e)
{
  return Expr(v) * e;
}

inline Expr operator/(Var const& v, double c)
{
  return Expr(v) / c;
}

inline Expr operator/(Var const& v, Expr const& e)
{
  return Expr(v) / e;
}

inline std::ostream& operator<<(std::ostream& os, Expr const& e)
{
  os << *e.p_impl.get();
  return os;
}

}  // namespace miplib
