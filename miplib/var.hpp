
#pragma once

#include <memory>
#include <functional>
#include <optional>

#include <boost/container_hash/hash.hpp>

namespace miplib {

struct Solver;

namespace detail {
struct IVar;
}  // namespace detail

struct Var
{
  enum class Type { Continuous, Binary, Integer };

  Var(
    Solver const& solver,
    Var::Type const& type,
    std::optional<double> const& lb = std::nullopt,
    std::optional<double> const& ub = std::nullopt,
    std::optional<std::string> const& name = std::nullopt
  );

  Var(Solver const& solver, Var::Type const& type, std::string const& name);

  Var(Var const& v) = default;

  bool is_same(Var const& v1) const;

  bool is_lex_less(Var const& v1) const;

  std::string id() const;

  std::optional<std::string> name() const;
  void set_name(std::string const& new_name);

  double value() const;

  template<class T> T value_as() const;

  Type type() const;

  Solver const& solver() const;

  double lb() const;
  double ub() const;

  void set_lb(double new_lb);
  void set_ub(double new_ub);

  std::shared_ptr<detail::IVar> p_impl;

  private:
  friend struct std::hash<Var>;
  friend struct boost::hash<Var>;
  friend struct std::less<Var>;
  friend struct GurobiSolver;
  friend struct ScipSolver;
  friend struct LpsolveSolver;  
  friend struct ScipCurrentStateHandle;
  friend struct ScipConstraintHandler;
  friend struct GurobiCurrentStateHandle;
};

template<class T>
T Var::value_as() const
{
  if (std::is_integral<T>::value)
  {
    return T(std::round(value()));
  }
  else
  return T(value());
}

std::ostream& operator<<(std::ostream& os, Var const& v);

namespace detail {

struct IVar
{
  virtual double value() const = 0;

  virtual Var::Type type() const = 0;

  virtual std::optional<std::string> name() const = 0;
  virtual void set_name(std::string const& new_name) = 0;

  virtual Solver const& solver() const = 0;

  virtual double lb() const = 0;
  virtual double ub() const = 0;

  virtual void set_lb(double new_lb) = 0;
  virtual void set_ub(double new_ub) = 0;
};

}  // namespace detail

}  // namespace miplib

// Default hash and comparison functions for Var.
namespace std {
template<>
struct hash<miplib::Var>
{
  std::size_t operator()(miplib::Var const& t) const noexcept
  {
    return std::hash<std::shared_ptr<miplib::detail::IVar>>()(t.p_impl);
  }
};


template<>
struct equal_to<miplib::Var>
{
  bool operator()(miplib::Var const& v1, miplib::Var const& v2) const noexcept
  {
    return v1.is_same(v2);
  }
};

template<>
struct less<miplib::Var>
{
  bool operator()(miplib::Var const& v1, miplib::Var const& v2) const noexcept
  {
    return v1.p_impl < v2.p_impl;
  }
};

}  // namespace std


// Default hash function for Var.
namespace boost {
template<class>
struct hash;
template<>
struct hash<miplib::Var>
{
  std::size_t operator()(miplib::Var const& t) const noexcept
  {
    return std::hash<std::shared_ptr<miplib::detail::IVar>>()(t.p_impl);
  }
};
}  // namespace boost

