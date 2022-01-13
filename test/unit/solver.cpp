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

  if (!Solver::backend_is_available(Backend))
  {
    WARN(fmt::format("Skipped since {} is not available.", Backend));
    return;
  }

  Solver solver(Backend, false);

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
  "Indicator constraint automatic reformulation", "[miplib]",
  ((miplib::Solver::Backend Backend), Backend),
  miplib::Solver::Backend::Gurobi,
  miplib::Solver::Backend::Scip,
  miplib::Solver::Backend::Lpsolve
)
{
  using namespace miplib;

  if (!Solver::backend_is_available(Backend))
  {
    WARN(fmt::format("Skipped since {} is not available.", Backend));
    return;
  }

  Solver solver(Backend);

  // Disable automatic reformulation
  solver.set_indicator_constraint_policy(Solver::IndicatorConstraintPolicy::PassThrough);

  SECTION("Can't add indicator constraint with inequation implicant")
  {
    Var z1(solver, Var::Type::Binary, "z1");
    Var z2(solver, Var::Type::Integer, "z2");
    REQUIRE_THROWS_AS(solver.add((z1 == 0) << (z2 >= 1)), std::logic_error);
  }

  SECTION("No backend supports indicator constraints with n-ary implicants")
  {
    Var z1(solver, Var::Type::Binary, "z1");
    Var z2(solver, Var::Type::Binary, "z2");
    REQUIRE_THROWS_AS(solver.add((z1 + z2 == 2) >> (z2 >= 1)), std::logic_error);
  }

  SECTION("No backend supports indicator constraints with non-linear implicand")
  {
    Var z1(solver, Var::Type::Binary, "z1");
    Var z2(solver, Var::Type::Binary, "z2");
    REQUIRE_THROWS_AS(solver.add((z1 == 0) >> (z2 * z1 >= 1)), std::logic_error);
  }

  SECTION("No backend supports indicator constraints with non-linear implicant")
  {
    Var z1(solver, Var::Type::Binary, "z1");
    Var z2(solver, Var::Type::Binary, "z2");
    REQUIRE_THROWS_AS(solver.add((z1 * z2 == 0) >> (z2 >= 1)), std::logic_error);
  }

  // Enable automatic reformulation
  solver.set_indicator_constraint_policy(Solver::IndicatorConstraintPolicy::ReformulateIfUnsupported);

  SECTION("Still can't add indicator constraint with inequation implicant")
  {
    Var z1(solver, Var::Type::Binary, "z1");
    Var z2(solver, Var::Type::Integer, "z2");
    REQUIRE_THROWS_AS(solver.add((z1 == 0) << (z2 >= 1)), std::logic_error);
  }

  SECTION("Half space implicants are automatically reformulated")
  {
    Var z1(solver, Var::Type::Binary, "z1");
    Var z2(solver, Var::Type::Binary, "z2");
    REQUIRE_NOTHROW(solver.add((z1 + z2 == 2) >> (z2 >= 1)));
  }

  SECTION("Non-linear implicands are automatically reformulated")
  {
    Var z1(solver, Var::Type::Binary, "z1");
    Var z2(solver, Var::Type::Binary, "z2");
    // lp solve doesn't support quadratic constraints.
    if (Backend != miplib::Solver::Backend::Lpsolve)
      REQUIRE_NOTHROW(solver.add((z1 == 0) >> (z2 * z1 >= 1)));
  }

  SECTION("Non-linear implicants are automatically reformulated")
  {
    Var z1(solver, Var::Type::Binary, "z1");
    Var z2(solver, Var::Type::Binary, "z2");
    // lp solve doesn't support quadratic constraints.
    if (Backend != miplib::Solver::Backend::Lpsolve)
      REQUIRE_NOTHROW(solver.add((z1 * z2 == 0) >> (z2 >= 1)));
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

  if (!Solver::backend_is_available(Backend))
  {
    WARN(fmt::format("Skipped since {} is not available.", Backend));
    return;
  }

  struct Handler : ILazyConstrHandler
  {
    Handler(Solver const& solver, Var const& v1, Var const& v2) : 
      m_solver(solver), m_v1(v1), m_v2(v2) {}
    virtual ~Handler() {}

    std::vector<Var> depends() const
    {
      return {m_v1, m_v2};
    }
    bool is_feasible()
    {
      return (m_v1.value_as<int>() != m_v2.value_as<int>());
    }
    bool add()
    {
      if (m_v1.value_as<int>() != m_v2.value_as<int>())
        return false;
      m_solver.add(m_v1 + m_v2 == 1);
      return true;
    }
    Solver m_solver;
    Var m_v1;
    Var m_v2;
  };


  SECTION("Test lazy constraint 1")
  {
    Solver solver(Backend, false);

    Var v1(solver, Var::Type::Integer, 0, 1, "v1");
    Var v2(solver, Var::Type::Integer, 0, 1, "v2");

    solver.add_lazy_constr_handler(LazyConstrHandler(std::make_shared<Handler>(solver, v1, v2)), true);

    auto [r, has_solution] = solver.maximize(v1);
    REQUIRE(r == Solver::Result::Optimal);
    REQUIRE(has_solution);
    REQUIRE(v1.value() + v2.value() == 1);
  }

  SECTION("Test lazy constraint 2")
  {
    Solver solver(Backend, false);

    Var v1(solver, Var::Type::Integer, 0, 1, "v1");
    Var v2(solver, Var::Type::Integer, 0, 1, "v2");

    solver.add_lazy_constr_handler(LazyConstrHandler(std::make_shared<Handler>(solver, v1, v2)), true);

    auto [r, has_solution] = solver.minimize(v1);
    REQUIRE(r == Solver::Result::Optimal);
    REQUIRE(has_solution);
    REQUIRE(v1.value() + v2.value() == 1);
  }
}

TEMPLATE_TEST_CASE_SIG(
  "Solver remove constraints", "[miplib]",
  ((miplib::Solver::Backend Backend), Backend),
  miplib::Solver::Backend::Gurobi,
  miplib::Solver::Backend::Scip
)
{
  using namespace miplib;

  if (!Solver::backend_is_available(Backend))
  {
    WARN(fmt::format("Skipped since {} is not available.", Backend));
    return;
  }

  Solver solver(Backend, false);

  Var v1(solver, Var::Type::Integer, 1, 2, "v1");
  Var v2(solver, Var::Type::Integer, 1, 2, "v2");

  SECTION("Test remove constraint")
  {
    auto c1 = v1 <= 1;
    auto c2 = v2 <= 1;
    solver.add(c1);
    solver.add(c2);
    auto [r, has_solution] = solver.maximize(v1 + v2);    
    
    REQUIRE(r == Solver::Result::Optimal);
    REQUIRE(has_solution);

    solver.remove(c1);
    std::tie(r, has_solution) = solver.maximize(v1 + v2);    
    REQUIRE(v1.value() == 2);
  }
}
