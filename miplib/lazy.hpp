#pragma once

#include <memory>
#include <miplib/var.hpp>

namespace miplib {

namespace detail {
struct ICurrentStateHandle
{
    virtual double value(IVar const& var) const = 0;
    virtual void add_lazy(Constr const& constr) = 0;
    virtual bool is_active() const = 0;
};
}

struct ILazyConstrHandler
{
  virtual bool is_feasible() = 0;
  // returns if at least one constraint was added to the model
  virtual bool add() = 0;
  virtual std::vector<Var> depends() const = 0;
};

struct LazyConstrHandler
{
  LazyConstrHandler(
    std::shared_ptr<ILazyConstrHandler> const& ap_impl
  ) : p_impl(ap_impl) {}
  LazyConstrHandler(LazyConstrHandler const& s) = default;


  std::vector<Var> depends() const { return p_impl->depends(); }
  bool is_feasible() { return p_impl->is_feasible(); }
  bool add() { return p_impl->add(); }

  private:
  std::shared_ptr<ILazyConstrHandler> p_impl;
};


// Convenience: A constraint handler that forwards handling to a passed object
// supporting the methods:
// bool lazy_constraints_feasible();
// bool lazy_constraints_add();
// std::vector<Var> lazy_constraints_depends() const;
template<class Handler>
struct DefaultLazyConstrHandler : ILazyConstrHandler
{
  DefaultLazyConstrHandler(Handler& handler) : m_handler(handler) {}
  bool is_feasible()
  {
    return m_handler.lazy_constraints_feasible();
  }
  bool add()
  {
    return m_handler.lazy_constraints_add();
  }
  std::vector<Var> depends() const
  {
    return m_handler.lazy_constraints_depends();
  }
  Handler& m_handler;
};

}
