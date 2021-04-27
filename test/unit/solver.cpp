#include <catch2/catch.hpp>

#include <miplib/solver.hpp>


#include <iostream>

#include <fmt/ostream.h>


TEMPLATE_TEST_CASE_SIG(
  "Solver (maximize/minimize)", "[miplib]",
  ((miplib::Solver::Backend Backend), Backend),
  miplib::Solver::Backend::Gurobi,
  miplib::Solver::Backend::Scip,
  miplib::Solver::Backend::Lpsolve
)
{
  using namespace miplib;

  if (!Solver::supports_backend(Backend))
    return;

  Solver solver(Backend);
  solver.set_verbose(false);

  Var v1(solver, Var::Type::Integer, 1, 3, "v1");
  Var v2(solver, Var::Type::Integer, 1, 3, "v2");

  SECTION("Test maximize variable")
  {
    auto [r, has_solution] = solver.maximize(v1);
    REQUIRE(r == Solver::Result::Optimal);
    REQUIRE(has_solution);
    REQUIRE(v1.value() == 3);
  }

  SECTION("Test maximize linear expression")
  {
    auto [r, has_solution] = solver.maximize(-v1 + v2);
    REQUIRE(r == Solver::Result::Optimal);
    REQUIRE(has_solution);
    REQUIRE(v1.value() == 1);
    REQUIRE(v2.value() == 3);
  }

  if (solver.supports_quadratic_objective())
    SECTION("Test maximize non-linear expression")
    {
      auto [r, has_solution] = solver.maximize(v1 * v2);
      REQUIRE(r == Solver::Result::Optimal);
      REQUIRE(has_solution);
      REQUIRE(v1.value() == 3);
      REQUIRE(v2.value() == 3);
    }

  SECTION("Test minimize variable")
  {
    auto [r, has_solution] = solver.minimize(v1);
    REQUIRE(r == Solver::Result::Optimal);
    REQUIRE(has_solution);
    REQUIRE(v1.value() == 1);
  }
}


TEMPLATE_TEST_CASE_SIG(
  "Lazy constraints", "[miplib]",
  ((miplib::Solver::Backend Backend), Backend),
  miplib::Solver::Backend::Gurobi,
  miplib::Solver::Backend::Scip//,
  //miplib::Solver::Backend::Lpsolve
)
{
  using namespace miplib;

  if (!Solver::supports_backend(Backend))
    return;

  struct Handler : ILazyConstrHandler
  {
    Handler(Var const& v1, Var const& v2) : 
      m_v1(v1), m_v2(v2) {}
    std::vector<Var> depends() const
    {
      return {m_v1, m_v2};
    }
    bool is_feasible(ICurrentStateHandle const& s)
    {
      return ((int)s.value(m_v1)) != ((int)s.value(m_v2));
    }
    bool add(ICurrentStateHandle& s)
    {
      if (s.value(m_v1) != s.value(m_v2))
        return false;
      s.add_lazy(m_v1 + m_v2 == 1);
      return true;
    }
    Var m_v1;
    Var m_v2;
  };


  SECTION("Test lazy constraint 1")
  {
    Solver solver(Backend);
    solver.set_verbose(false);

    Var v1(solver, Var::Type::Integer, 0, 1, "v1");
    Var v2(solver, Var::Type::Integer, 0, 1, "v2");

    solver.set_lazy_constr_handler(LazyConstrHandler(std::make_shared<Handler>(v1, v2)));

    auto [r, has_solution] = solver.maximize(v1);
    REQUIRE(r == Solver::Result::Optimal);
    REQUIRE(has_solution);
    REQUIRE(v1.value() + v2.value() == 1);
  }

  SECTION("Test lazy constraint 2")
  {
    Solver solver(Backend);
    solver.set_verbose(false);

    Var v1(solver, Var::Type::Integer, 0, 1, "v1");
    Var v2(solver, Var::Type::Integer, 0, 1, "v2");

    solver.set_lazy_constr_handler(LazyConstrHandler(std::make_shared<Handler>(v1, v2)));

    auto [r, has_solution] = solver.minimize(v1);
    REQUIRE(r == Solver::Result::Optimal);
    REQUIRE(has_solution);
    REQUIRE(v1.value() + v2.value() == 1);
  }
}