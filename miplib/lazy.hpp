#pragma once

#include <memory>
#include <miplib/var.hpp>

namespace miplib {

struct ICurrentStateHandle
{
    virtual double value(Var const& var) const = 0;
    virtual void add_lazy(Constr const& constr) = 0;
};

struct ILazyConstrHandler
{
  virtual bool is_feasible(ICurrentStateHandle const& s) = 0;
  // returns if at least one constraint was added to the model
  virtual bool add(ICurrentStateHandle& s) = 0;
  virtual std::vector<Var> depends() const = 0;
};

struct LazyConstrHandler
{
  LazyConstrHandler(
    std::shared_ptr<ILazyConstrHandler> const& ap_impl
  ) : p_impl(ap_impl) {}
  LazyConstrHandler(LazyConstrHandler const& s) = default;


  std::vector<Var> depends() const { return p_impl->depends(); }
  bool is_feasible(ICurrentStateHandle const& s) { return p_impl->is_feasible(s); }
  bool add(ICurrentStateHandle& s) { return p_impl->add(s); }

  private:
  std::shared_ptr<ILazyConstrHandler> p_impl;
};

}