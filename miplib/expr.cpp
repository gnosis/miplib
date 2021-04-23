
#include "expr.hpp"
#include "solver.hpp"

#include <fmt/ostream.h>

namespace miplib {
namespace detail {

VarPair ordered(VarPair const& vp)
{
  VarPair r(vp);
  if (!vp.first.is_lex_less(vp.second))
  {
    Var t = r.first;
    r.first = r.second;
    r.second = t;
  }
  return r;
}

VarPair ordered_by_name(VarPair const& vp)
{
  VarPair r(vp);
  if (vp.first.id() > vp.second.id())
  {
    Var t = r.first;
    r.first = r.second;
    r.second = t;
  }
  return r;
}

Solver const& ExprImpl::solver() const
{
  if (!m_linear.empty())
  {
    return m_linear.begin()->first.solver();
  }
  else if (!m_quad.empty())
  {
    return m_quad.begin()->first.first.solver();
  }
  else
  {
    throw std::logic_error(
      fmt::format("Attempt to access solver from constant expression {}.", *this)
    );
  }
}

ExprImpl& ExprImpl::operator+=(double c)
{
  m_constant += c;
  return *this;
}


ExprImpl& ExprImpl::operator+=(Var v)
{
  auto mit = m_linear.find(v);
  if (mit == m_linear.end())
  {
    m_linear.insert(mit, {v, 1});
  }
  else
  {
    mit->second += 1;
    if (mit->second == 0)
    {
      m_linear.erase(mit);
    }
  }
  return *this;
}

ExprImpl& ExprImpl::operator+=(ExprImpl const& e)
{
  m_constant += e.m_constant;

  // add linear term coefficients
  for (auto& [v, c]: e.m_linear)
  {
    auto mit = m_linear.find(v);
    if (mit == m_linear.end())
    {
      m_linear.insert(mit, {v, c});
    }
    else
    {
      mit->second += c;
      if (mit->second == 0)
      {
        m_linear.erase(mit);
      }
    }
  }

  // add quad term coefficients
  for (auto& [v1v2, c]: e.m_quad)
  {
    auto mit = m_quad.find(v1v2);
    if (mit == m_quad.end())
    {
      m_quad.insert(mit, {v1v2, c});
    }
    else
    {
      mit->second += c;
      if (mit->second == 0)
      {
        m_quad.erase(mit);
      }
    }
  }

  return *this;
}

ExprImpl& ExprImpl::operator-=(Var v)
{
  auto mit = m_linear.find(v);
  if (mit == m_linear.end())
  {
    m_linear.insert(mit, {v, -1});
  }
  else
  {
    mit->second -= 1;
    if (mit->second == 0)
    {
      m_linear.erase(mit);
    }
  }
  return *this;
}


ExprImpl& ExprImpl::operator*=(double c)
{
  if (c == 0)
  {
    m_quad.clear();
    m_linear.clear();
    m_constant = 0;
    return *this;
  }
  // multiply by quad coeffs
  for (auto& [v1v2, c1]: m_quad) { c1 *= c; }
  // multiply by linear coeffs
  for (auto& [v1, c1]: m_linear) { c1 *= c; }
  // multiply by const coeffs
  m_constant *= c;
  return *this;
}

ExprImpl& ExprImpl::operator*=(Var v)
{
  return *this *= ExprImpl(v);
}

ExprImpl& ExprImpl::operator*=(ExprImpl const& e)
{
  if (!m_quad.empty() and !e.m_quad.empty())
  {
    throw std::logic_error(
      fmt::format("Attempt to create quartic expression {} * {}.", *this, e)
    );
  }
  if (
    (!m_quad.empty() and !e.m_linear.empty()) or 
    (!m_linear.empty() and !e.m_quad.empty())
  )
  {
    throw std::logic_error(
      fmt::format("Attempt to create cubic expression {} * {}.", *this, e)
    );
  }

  // backup original constant and linear terms
  auto const constant = m_constant;
  auto const linear = m_linear;

  // multiply e constant of with all
  *this *= e.m_constant;

  // multiply original constant with linear terms of e
  for (auto const& [v1, c1]: e.m_linear)
  {
    auto mit = m_linear.find(v1);
    if (mit == m_linear.end())
    {
      if (constant != 0)
      {
        m_linear.insert(mit, {v1, constant * c1});
      }
    }
    else
    {
      mit->second += constant * c1;
      if (mit->second == 0)
        m_linear.erase(mit);
    }
  }

  // multiply original constant with quad terms of e
  for (auto const& [v1v2, c1]: e.m_quad)
  {
    auto mit = m_quad.find(v1v2);
    if (mit == m_quad.end())
    {
      if (constant != 0)
      {
        m_quad.insert(mit, {v1v2, constant * c1});
      }
    }
    else
    {
      mit->second += constant * c1;
      if (mit->second == 0)
        m_quad.erase(mit);
    }
  }

  // multiply original linear terms with linear terms of e
  for (auto const& [v1, c1]: linear)
    for (auto const& [v2, c2]: e.m_linear)
    {
      auto v1v2 = ordered({v1, v2});
      auto mit = m_quad.find(v1v2);
      if (mit == m_quad.end())
        m_quad.insert(mit, {v1v2, c1 * c2});
      else
      {
        mit->second += c1 * c2;
        if (mit->second == 0)
          m_quad.erase(mit);
      }
    }

  return *this;
}

ExprImpl& ExprImpl::operator/=(double c)
{
  return *this *= 1 / c;
}


std::ostream& operator<<(std::ostream& os, ExprImpl const& e)
{
  // sort quad terms by id (for tests)
  std::vector<std::pair<VarPair, double>> quad(e.m_quad.begin(), e.m_quad.end());

  for (auto& [vp, c]: quad) vp = ordered_by_name(vp);

  std::sort(quad.begin(), quad.end(), [](auto const& vpc1, auto const& vpc2) {
    if (vpc1.first.first.id() != vpc2.first.first.id())
      return vpc1.first.first.id() < vpc2.first.first.id();
    return vpc1.first.second.id() < vpc2.first.second.id();
  });

  // sort linear terms by id (for tests)
  std::vector<std::pair<Var, double>> linear(e.m_linear.begin(), e.m_linear.end());
  std::sort(linear.begin(), linear.end(), [](auto const& vc1, auto const& vc2) {
    return vc1.first.id() < vc2.first.id();
  });

  bool first_term = true;
  auto print_coefficient = [&](double c, bool is_const = false) {
    if (!first_term)
      os << " ";
    if (c < 0)
      os << "-";
    else if (c > 0 and !first_term)
      os << "+";
    if (!first_term)
      os << " ";
    if (!is_const and c != 1 and c != -1)
      os << std::abs(c) << " ";
    if (is_const)
      os << std::abs(c);
    first_term = false;
  };

  for (auto const& [v1v2, c]: quad)
  {
    auto const& [v1, v2] = v1v2;
    print_coefficient(c);
    os << v1 << " " << v2;
  }
  for (auto const& [v, c]: linear)
  {
    print_coefficient(c);
    os << v;
  }
  // avoid printing trailing 0
  if (e.m_constant == 0 and !first_term)
  {
    return os;
  }
  print_coefficient(e.m_constant, true);
  return os;
}

}  // namespace detail


bool Expr::must_be_binary() const
{
  if (is_constant())
    return constant() == 0 or constant() == 1;

  auto const& impl = *p_impl;

  if (impl.m_linear.size() > 1)
    return false;

  if (impl.m_quad.size() > 1)
    return false;

  if (impl.m_linear.size() + impl.m_quad.size() > 1)
    return false;

  if (is_linear())
  {
    if (impl.m_linear.begin()->first.type() != Var::Type::Binary)
      return false;

    if (impl.m_constant == 1 and impl.m_linear.begin()->second == -1)
      return true;

    return impl.m_constant == 0 and impl.m_linear.begin()->second == 1;
  }
  else
  {
    if (impl.m_quad.begin()->first.first.type() != Var::Type::Binary)
      return false;

    if (impl.m_quad.begin()->first.second.type() != Var::Type::Binary)
      return false;

    if (impl.m_constant == 1 and impl.m_quad.begin()->second == -1)
      return true;

    return impl.m_constant == 0 and impl.m_quad.begin()->second == 1;
  }
}


// coefficients of linear part
std::vector<double> Expr::linear_coeffs() const
{
  std::vector<double> r;
  std::transform(
    p_impl->m_linear.begin(),
    p_impl->m_linear.end(),
    std::back_inserter(r),
    [](auto const& vc) { return vc.second; }
  );
  return r;
}

// variables of linear part
std::vector<Var> Expr::linear_vars() const
{
  std::vector<Var> r;
  std::transform(
    p_impl->m_linear.begin(),
    p_impl->m_linear.end(),
    std::back_inserter(r),
    [](auto const& vc) { return vc.first; }
  );
  return r;
}

// coefficients of quad part
std::vector<double> Expr::quad_coeffs() const
{
  std::vector<double> r;
  std::transform(
    p_impl->m_quad.begin(),
    p_impl->m_quad.end(),
    std::back_inserter(r),
    [](auto const& vvc) { return vvc.second; }
  );
  return r;
}

// variables of quad part
std::vector<Var> Expr::quad_vars_1() const
{
  std::vector<Var> r;
  std::transform(
    p_impl->m_quad.begin(),
    p_impl->m_quad.end(),
    std::back_inserter(r),
    [](auto const& vvc) { return vvc.first.first; }
  );
  return r;
}

// variables of quad part
std::vector<Var> Expr::quad_vars_2() const
{
  std::vector<Var> r;
  std::transform(
    p_impl->m_quad.begin(),
    p_impl->m_quad.end(),
    std::back_inserter(r),
    [](auto const& vvc) { return vvc.first.second; }
  );
  return r;
}

// Returns the lower and upper bounds of a term.
static std::pair<double, double> linear_term_bounds(Var const& v, double coeff)
{
  auto const var_lb = v.lb();
  auto const var_ub = v.ub();

  double const inf = v.solver().infinity();

  if (coeff > 0)
    return std::make_pair(std::max(-inf, coeff * var_lb), std::min(inf, coeff * var_ub));
  else
  if (coeff < 0)
    return std::make_pair(std::max(-inf, coeff * var_ub), std::min(inf, coeff * var_lb));
  else
    return std::make_pair(0, 0);
}


// Returns the lower and upper bounds of a linear expression.
std::pair<double, double> Expr::bounds() const
{
  if (!is_linear()) 
    throw std::logic_error(
      fmt::format("Querying bounds of a nonlinear expression is not implemented yet: {}.", *this)
    );

  auto const lin_coeffs = linear_coeffs();
  auto const lin_vars = linear_vars();

  double lb = constant();
  double ub = constant();

  double const inf = solver().infinity();

  for (std::size_t i = 0; i < lin_coeffs.size(); ++i)
  {
    auto const [term_lb, term_ub] = linear_term_bounds(lin_vars[i], lin_coeffs[i]);
    lb += term_lb;
    ub += term_ub;
  }
  return std::make_pair(std::max(-inf, lb), std::min(inf, ub));
}

// Returns the maximum absolute value of the term with lowest maximum absolute value and
// the maximum absolute value of the term with highest maximum absolute value.
std::pair<double, double> Expr::numerical_range() const
{
  auto lin_coeffs = linear_coeffs();
  auto lin_vars = linear_vars();

  double lb = abs(constant());
  double ub = abs(constant());

  for (std::size_t i = 0; i < lin_coeffs.size(); ++i)
  {
    auto const [term_lb, term_ub] = linear_term_bounds(lin_vars[i], lin_coeffs[i]);
    double max_abs = std::max(abs(term_lb), abs(term_ub));
    lb = std::min(lb, max_abs);
    ub = std::max(ub, max_abs);
  }
  return std::make_pair(lb, ub);
}

Expr Expr::copy() const
{
  Expr r;
  r.p_impl = std::make_shared<detail::ExprImpl>(*p_impl);
  return r;
}

Expr Expr::operator-() const
{
  Expr r(copy());
  return r *= -1;
}

Expr Expr::operator-(Var const& v) const
{
  return *this + (-v);
}

Expr Expr::operator+(double c) const
{
  Expr r(copy());
  return r += c;
}

Expr Expr::operator+(Var const& v) const
{
  Expr r(copy());
  return r += v;
}

Expr Expr::operator+(Expr const& e) const
{
  Expr r(copy());
  return r += e;
}

Expr Expr::operator*(double c) const
{
  Expr r(copy());
  return r *= c;
}

Expr Expr::operator*(Var const& v) const
{
  Expr r(copy());
  return r *= v;
}

Expr Expr::operator*(Expr const& e) const
{
  Expr r(copy());
  return r *= e;
}

Expr Expr::operator/(Expr const& e) const
{
  if (!e.is_constant())
  {
    throw std::logic_error(
      fmt::format("Attempt to divide non-constant expression {} / {}.", *this, e));
  }
  return (*this) / e.constant();
}

// computes the lower bound of the product of two intervals using interval arithmetic
// is_same should be true if the two intervals are the domain of the same variable
// (in which case the lb can be tightened)
static double interval_prod_lb(
  double lb1, double ub1, double lb2, double ub2, bool is_same
)
{
  // P * P
  if (lb1 >= 0 and lb2 >= 0)
    return lb1 * lb2;
  else
  // N * N
  if (ub1 <= 0 and ub2 <= 0)
    return ub1 * ub2;
  else
  if (is_same)
    return 0;
  else
  {
    return std::min(lb1 * ub2, ub1 * lb2);
  }
}


// computes the upper bound of the product of two intervals using interval arithmetic
static double interval_prod_ub(
  double lb1, double ub1, double lb2, double ub2
)
{
  // P * P
  if (lb1 >= 0 and lb2 >= 0)
    return ub1 * ub2;
  else
  // N * N
  if (ub1 <= 0 and ub2 <= 0)
    return lb1 * lb2;
  else
    return std::max(lb1 * lb2, ub1 * ub2);
}


double Expr::lb() const
{
  double infinity = solver().infinity();

  double r = constant();

  // linear part
  auto const lv = linear_vars();
  auto const lc = linear_coeffs();
  for (std::size_t i = 0; i < lc.size(); ++i)
    if (lc[i] > 0)
    {
      if (lv[i].lb() == -infinity)
        return -infinity;
      else
        r += lc[i] * lv[i].lb();
    }
    else
    if (lc[i] < 0)
    {
      if (lv[i].ub() == infinity)
        return -infinity;
      else
        r += lc[i] * lv[i].ub();
    }

  // quadratic part
  auto const qv1 = quad_vars_1();
  auto const qv2 = quad_vars_2();
  auto const qc = quad_coeffs();
  for (std::size_t i = 0; i < qc.size(); ++i)
    if (qc[i] > 0)
    {
      double lb = interval_prod_lb(
        qv1[i].lb(), qv1[i].ub(), qv2[i].lb(), qv2[i].ub(),
        qv1[i].is_same(qv2[i])
      );      
      if (lb <= -infinity)
        return -infinity;
      r += qc[i] * lb;
    }
    else
    if (qc[i] < 0)
    {
      double ub = interval_prod_ub(
        qv1[i].lb(), qv1[i].ub(), qv2[i].lb(), qv2[i].ub()
      );      
      if (ub >= infinity)
        return -infinity;
      r += qc[i] * ub;
    }

  return r;
}

double Expr::ub() const
{
  return -(-(*this)).lb();
}

std::vector<Var> Expr::vars() const
{
  std::set<Var> r;
  for (auto const& [v, c]: p_impl->m_linear)
    r.insert(v);
  for (auto const& [vv, c]: p_impl->m_quad)
  {
    r.insert(vv.first);
    r.insert(vv.second);
  }
  return std::vector(r.begin(), r.end());
}

std::size_t Expr::arity() const
{
  return vars().size();
}


}  // namespace miplib
