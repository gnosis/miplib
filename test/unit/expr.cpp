#include <catch2/catch.hpp>

#include <miplib/solver.hpp>
#include <miplib/expr.hpp>

#include <iostream>

#include <fmt/ostream.h>


TEMPLATE_TEST_CASE_SIG(
  "Expressions", "[miplib]",
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

  Var v1(solver, Var::Type::Binary, "v1");
  Var v2(solver, Var::Type::Binary, "v2");
  Var v3(solver, Var::Type::Continuous, "v3");

  REQUIRE(fmt::format("{}", Expr(3)) == "3");
  REQUIRE(fmt::format("{}", -v1) == "-v1");
  REQUIRE(fmt::format("{}", v1 - v2) == "v1 - v2");
  REQUIRE(fmt::format("{}", 2 * v1 - v2) == "2 v1 - v2");
  REQUIRE(fmt::format("{}", 2 * v1 - 2 * v1) == "0");
  REQUIRE(fmt::format("{}", 2 * (v1 - v2)) == "2 v1 - 2 v2");
  REQUIRE(fmt::format("{}", 2 * (v1 - v2) + v1) == "3 v1 - 2 v2");
  REQUIRE(fmt::format("{}", 2 * (v1 - v2) - 2 * v1) == "-2 v2");
  REQUIRE(fmt::format("{}", 2 * (v1 - v2) / 2) == "v1 - v2");
  REQUIRE(fmt::format("{}", v1 * v2) == "v1 v2");
  REQUIRE(fmt::format("{}", (v1 + v2) * (v2 + v3)) == "v1 v2 + v1 v3 + v2 v2 + v2 v3");
  REQUIRE(fmt::format("{}", (v1 + v2) * (v1 - v2)) == "v1 v1 - v2 v2");
  REQUIRE(
    fmt::format("{}",
      (1 * v1 + 2 * v2) * (3 * v2 + 4 * v3)
    ) == "3 v1 v2 + 4 v1 v3 + 6 v2 v2 + 8 v2 v3"
  );

  // Cubic expression.
  REQUIRE_THROWS_AS(v1 * v2 * v3, std::logic_error);
  REQUIRE_THROWS_AS((v1 + 2) * (v2 + 2) * v3, std::logic_error);

  // Quartic expression.
  REQUIRE_THROWS_AS(v1 * v1 * v1 * v1, std::logic_error);
}


TEMPLATE_TEST_CASE_SIG(
  "Constraints", "[miplib]",
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

  Var v1(solver, Var::Type::Binary, "v1");
  Var v2(solver, Var::Type::Binary, "v2");
  Var v3(solver, Var::Type::Integer, "v3");

  // Linear constraints.
  REQUIRE(fmt::format("{}", v1 == v2) == "v1 - v2 = 0");
  REQUIRE(fmt::format("{}", v1 >= v2) == "-v1 + v2 <= 0");
  REQUIRE(fmt::format("{}", v1 >= 1) == "-v1 + 1 <= 0");

  // Quadratic constraints.
  REQUIRE(fmt::format("{}", v1 * v2 >= 1) == "-v1 v2 + 1 <= 0");
  REQUIRE(
    fmt::format("{}",
      (1 * v1 + 2 * v2) * (3 * v2 + 4 * v3) == 2
    ) == "3 v1 v2 + 4 v1 v3 + 6 v2 v2 + 8 v2 v3 - 2 = 0"
  );

  // Non binary negation.
  REQUIRE_THROWS_AS(!v3, std::logic_error);
  REQUIRE_THROWS_AS(!(v1 + 1), std::logic_error);
  REQUIRE_THROWS_AS(!(2 * v1), std::logic_error);

  // But this is fine:
  REQUIRE(fmt::format("{}", !v1) == "v1 = 0");
  REQUIRE(fmt::format("{}", !(1 - v1)) == "-v1 + 1 = 0");

  // Indicator constraints.
  if (!solver.supports_indicator_constraints()) return;

  REQUIRE(fmt::format("{}", (v1 == 0) >> (v2 >= 1)) == "v1 = 0 -> -v2 + 1 <= 0");
  REQUIRE(fmt::format("{}", !v1 >> (v2 >= 1)) == "v1 = 0 -> -v2 + 1 <= 0");
  REQUIRE(fmt::format("{}", (2 * v1 == 0) >> (v2 >= 1)) == "2 v1 = 0 -> -v2 + 1 <= 0");
  REQUIRE(fmt::format("{}", (v1 == 0) << (v2 == 1)) == "v2 - 1 = 0 -> v1 = 0");
  REQUIRE(fmt::format("{}", !v1 << v2) == "v2 - 1 = 0 -> v1 = 0");
  REQUIRE(
    fmt::format("{}",
      (v1 == 1) >> (2*v2 + v3 <= 1)) == "v1 - 1 = 0 -> 2 v2 + v3 - 1 <= 0"
  );

  // Implicant must be an equation v=1 or v=0.
  REQUIRE_THROWS_AS((v1 == 0) << (v2 >= 1), std::logic_error);
  REQUIRE_THROWS_AS((v1 + v2 == 0) >> (v2 >= 1), std::logic_error);
  REQUIRE_THROWS_AS((v1 * v2 == 0) >> (v2 >= 1), std::logic_error);

  // Implicand must be linear.
  REQUIRE_THROWS_AS((v1 == 0) >> (v2 * v1 >= 1), std::logic_error);
}

TEMPLATE_TEST_CASE_SIG(
  "Solver (linear constraints)", "[miplib]",
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

  Var v1(solver, Var::Type::Binary, "v1");
  Var v2(solver, Var::Type::Binary, "v2");
  Var v3(solver, Var::Type::Continuous, "v3");

  solver.add(v1 == 1);
  solver.add(v2 <= v1 - 1);
  solver.add(v3 == v1 + v2);

  auto r = solver.solve();

  REQUIRE(r == Solver::Result::Optimal);

  REQUIRE(v1.value() == 1);
  REQUIRE(v2.value() == 0);
  REQUIRE(v3.value() == 1);
}

TEMPLATE_TEST_CASE_SIG(
  "Solver (non-convex constraints)", "[miplib]",
  ((miplib::Solver::Backend Backend), Backend),
  miplib::Solver::Backend::Gurobi, miplib::Solver::Backend::Scip
)
{
  using namespace miplib;

  if (!Solver::supports_backend(Backend))
    return;

  Solver solver(Backend);

  solver.set_verbose(false);
  solver.set_non_convex_policy(Solver::NonConvexPolicy::Branch);

  Var v1(solver, Var::Type::Continuous, "v1");
  Var v2(solver, Var::Type::Continuous, "v2");

  solver.add(v1 * v2 == 0.5);
  solver.add(v1 >= 0.707);
  solver.add(v2 >= 0.707);

  auto r = solver.solve();

  REQUIRE(r == Solver::Result::Optimal);
}

TEMPLATE_TEST_CASE_SIG(
  "Solver (indicator constraints)", "[miplib]",
  ((miplib::Solver::Backend Backend), Backend),
  miplib::Solver::Backend::Gurobi, miplib::Solver::Backend::Scip
)
{
  using namespace miplib;

  if (!Solver::supports_backend(Backend))
    return;

  Solver solver(Backend);
  solver.set_verbose(false);

  Var v1(solver, Var::Type::Binary, "v1");
  Var v2(solver, Var::Type::Binary, "v2");
  Var v3(solver, Var::Type::Binary, "v3");

  solver.add((v1 == 1) >> (v2 == v3));
  solver.add((v1 == 0) >> (v2 == v3 - 1));
  solver.add(v2 <= v3 - 1);

  auto r = solver.solve();

  REQUIRE(r == Solver::Result::Optimal);

  REQUIRE(v1.value() == 0);
  REQUIRE(v2.value() == 0);
  REQUIRE(v3.value() == 1);
}



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
    auto r = solver.maximize(v1);
    REQUIRE(r == Solver::Result::Optimal);
    REQUIRE(v1.value() == 3);
  }

  SECTION("Test maximize linear expression")
  {
    auto r = solver.maximize(-v1 + v2);
    REQUIRE(r == Solver::Result::Optimal);
    REQUIRE(v1.value() == 1);
    REQUIRE(v2.value() == 3);
  }

  if (solver.supports_quadratic_objective())
    SECTION("Test maximize non-linear expression")
    {
      auto r = solver.maximize(v1 * v2);
      REQUIRE(r == Solver::Result::Optimal);
      REQUIRE(v1.value() == 3);
      REQUIRE(v2.value() == 3);
    }

  SECTION("Test minimize variable")
  {
    auto r = solver.minimize(v1);
    REQUIRE(r == Solver::Result::Optimal);
    REQUIRE(v1.value() == 1);
  }
}
