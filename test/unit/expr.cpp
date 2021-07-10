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

  if (!Solver::backend_is_available(Backend))
  {
    WARN(fmt::format("Skipped since {} is not available.", Backend));
    return;
  }

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

  if (!Solver::backend_is_available(Backend))
  {
    WARN(fmt::format("Skipped since {} is not available.", Backend));
    return;
  }

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
  REQUIRE(fmt::format("{}", (v1 == 0) >> (v2 >= 1)) == "v1 = 0 -> -v2 + 1 <= 0");
  REQUIRE(fmt::format("{}", !v1 >> (v2 >= 1)) == "v1 = 0 -> -v2 + 1 <= 0");
  REQUIRE(fmt::format("{}", (2 * v1 == 0) >> (v2 >= 1)) == "2 v1 = 0 -> -v2 + 1 <= 0");
  REQUIRE(fmt::format("{}", (v1 == 0) << (v2 == 1)) == "v2 - 1 = 0 -> v1 = 0");
  REQUIRE(fmt::format("{}", !v1 << v2) == "v2 - 1 = 0 -> v1 = 0");
  REQUIRE(
    fmt::format("{}",
      (v1 == 1) >> (2*v2 + v3 <= 1)) == "v1 - 1 = 0 -> 2 v2 + v3 - 1 <= 0"
  );
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

  if (!Solver::backend_is_available(Backend))
  {
    WARN(fmt::format("Skipped since {} is not available.", Backend));
    return;
  }

  Solver solver(Backend);
  solver.set_verbose(false);

  Var v1(solver, Var::Type::Binary, "v1");
  Var v2(solver, Var::Type::Binary, "v2");
  Var v3(solver, Var::Type::Continuous, "v3");

  solver.add(v1 == 1);
  solver.add(v2 <= v1 - 1);
  solver.add(v3 == v1 + v2);

  auto [r, has_solution] = solver.solve();

  REQUIRE(r == Solver::Result::Optimal);
  REQUIRE(has_solution);

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

  if (!Solver::backend_is_available(Backend))
  {
    WARN(fmt::format("Skipped since {} is not available.", Backend));
    return;
  }

  Solver solver(Backend);

  solver.set_verbose(false);
  solver.set_non_convex_policy(Solver::NonConvexPolicy::Branch);

  Var v1(solver, Var::Type::Continuous, "v1");
  Var v2(solver, Var::Type::Continuous, "v2");

  solver.add(v1 * v2 == 0.5);
  solver.add(v1 >= 0.707);
  solver.add(v2 >= 0.707);

  auto [r, has_solution] = solver.solve();

  REQUIRE(r == Solver::Result::Optimal);
  REQUIRE(has_solution);
}

TEMPLATE_TEST_CASE_SIG(
  "Solver (indicator constraints)", "[miplib]",
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
  solver.set_verbose(false);

  Var v1(solver, Var::Type::Binary, "v1");
  Var v2(solver, Var::Type::Binary, "v2");
  Var v3(solver, Var::Type::Binary, "v3");

  solver.add((v1 == 1) >> (v2 == v3));
  solver.add((v1 == 0) >> (v2 == v3 - 1));
  solver.add(v2 <= v3 - 1);

  auto [r, has_solution] = solver.solve();

  REQUIRE(r == Solver::Result::Optimal);
  REQUIRE(has_solution);

  REQUIRE(v1.value() == 0);
  REQUIRE(v2.value() == 0);
  REQUIRE(v3.value() == 1);
}


TEMPLATE_TEST_CASE_SIG(
  "Lower/upper bounds", "[miplib]",
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

  Var v1(solver, Var::Type::Binary);
  REQUIRE(v1.lb() == 0);
  REQUIRE(v1.ub() == 1);

  Var v2(solver, Var::Type::Integer);
  REQUIRE(v2.lb() == -solver.infinity());
  REQUIRE(v2.ub() == solver.infinity());

  Var v3(solver, Var::Type::Continuous);
  REQUIRE(v3.lb() == -solver.infinity());
  REQUIRE(v3.ub() == solver.infinity());

  Var v4(solver, Var::Type::Continuous, 1, 3);
  REQUIRE(v4.lb() == 1);
  REQUIRE(v4.ub() == 3);

  Var v5(solver, Var::Type::Continuous, -1, 3);
  REQUIRE(v5.lb() == -1);
  REQUIRE(v5.ub() == 3);

  Var v6(solver, Var::Type::Integer, -1);  
  REQUIRE(v6.lb() == -1);
  REQUIRE(v6.ub() == solver.infinity());

  REQUIRE((2*v1).lb() == 0);
  REQUIRE((2*v1).ub() == 2);

  REQUIRE((2*v1 + 3).lb() == 3);
  REQUIRE((2*v1 + 3).ub() == 5);

  REQUIRE((v1+v2).lb() == -solver.infinity());
  REQUIRE((v1+v2).ub() == solver.infinity());

  REQUIRE((v2+v3).lb() == -solver.infinity());
  REQUIRE((v2+v3).ub() == solver.infinity());

  REQUIRE((2*v4 - v5).lb() == -1);
  REQUIRE((2*v4 - v5).ub() == 7);

  REQUIRE((v1*v1).lb() == 0);
  REQUIRE((v1*v1).ub() == 1);

  REQUIRE((v1*v2).lb() == -solver.infinity());
  REQUIRE((v1*v2).ub() == solver.infinity());

  REQUIRE((v2*v3).lb() == -solver.infinity());
  REQUIRE((v2*v3).ub() == solver.infinity());

  REQUIRE((v4*v5).lb() == -3);
  REQUIRE((v4*v5).ub() == 9);

  REQUIRE((v5*v5).lb() == 0);
  REQUIRE((v5*v5).ub() == 9);

  REQUIRE((v5*v6).lb() == -solver.infinity());
  REQUIRE((v5*v6).ub() == solver.infinity());

  v4.set_ub(2);
  REQUIRE((v4*v5).lb() == -2);
  REQUIRE((v4*v5).ub() == 6);
}


TEMPLATE_TEST_CASE_SIG(
  "Indicator constraint reformulation", "[miplib]",
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

  Var z(solver, Var::Type::Binary, "z");

  SECTION("No reformulation since unbounded")
  {
    Var x(solver, Var::Type::Integer, "x");
    REQUIRE(!(z >> (x <= 0)).has_reformulation());
  }

  SECTION("No reformulation since unbounded above")
  {
    Var x(solver, Var::Type::Integer, -1, std::nullopt, "x");
    REQUIRE(!(z >> (x <= 0)).has_reformulation());
  }

  SECTION("No reformulation since unbounded below")
  {
    Var x(solver, Var::Type::Integer, std::nullopt, 1, "x");
    REQUIRE(!(z >> (x == 0)).has_reformulation());
  }

  SECTION("Inequality reformulation")
  {
    Var x(solver, Var::Type::Integer, std::nullopt, 2, "x");
    REQUIRE((z >> (x <= 0)).has_reformulation());
    auto const r = (z >> (x <= 0)).reformulation();
    REQUIRE(r.size() == 1);
    REQUIRE(fmt::format("{}", r[0]) == "x + 2 z - 2 <= 0");
  }

  SECTION("Inequality reformulation #2")
  {
    Var x(solver, Var::Type::Integer, std::nullopt, 2, "x");
    REQUIRE(((z==1) >> (x <= 0)).has_reformulation());
    auto const r = ((z==1) >> (x <= 0)).reformulation();
    REQUIRE(r.size() == 1);
    REQUIRE(fmt::format("{}", r[0]) == "x + 2 z - 2 <= 0");
  }

  SECTION("Negated inequality reformulation")
  {
    Var x(solver, Var::Type::Integer, std::nullopt, 2, "x");
    REQUIRE((!z >> (x <= 0)).has_reformulation());
    auto const r = (!z >> (x <= 0)).reformulation();
    REQUIRE(r.size() == 1);
    REQUIRE(fmt::format("{}", r[0]) == "x - 2 z <= 0");
  }

  SECTION("Equality reformulation")
  {
    Var x(solver, Var::Type::Integer, 2, 4, "x");
    REQUIRE((z >> (x == 3)).has_reformulation());
    auto const r = (z >> (x == 3)).reformulation();
    REQUIRE(r.size() == 2);
    REQUIRE(fmt::format("{}", r[0]) == "x + z - 4 <= 0");
    REQUIRE(fmt::format("{}", r[1]) == "-x + z + 2 <= 0");
  }

  SECTION("Equality reformulation (only one side)")
  {
    Var x(solver, Var::Type::Integer, 2, 4, "x");
    REQUIRE((z >> (x == 2)).has_reformulation());
    auto const r = (z >> (x == 2)).reformulation();
    REQUIRE(r.size() == 1);
    REQUIRE(fmt::format("{}", r[0]) == "x + 2 z - 4 <= 0");
  }

  SECTION("No reformulation since implicant is not a half space")
  {
    Var z1(solver, Var::Type::Binary, "z1");
    Var z2(solver, Var::Type::Binary, "z2");
    Var x(solver, Var::Type::Integer, std::nullopt, 1, "x");
    REQUIRE(!((z1 - z2) >> (x == 0)).has_reformulation());
  }

  SECTION("Inequality reformulation of non unary implicant")
  {
    Var z1(solver, Var::Type::Binary, "z1");
    Var z2(solver, Var::Type::Binary, "z2");
    Var x(solver, Var::Type::Integer, std::nullopt, 2, "x");
    REQUIRE(((z1 + z2 == 2) >> (x <= 0)).has_reformulation());
    auto const r = ((z1 + z2 == 2) >> (x <= 0)).reformulation();
    REQUIRE(r.size() == 1);
    REQUIRE(fmt::format("{}", r[0]) == "x + 2 z1 + 2 z2 - 4 <= 0");
  }

  SECTION("Inequality reformulation of non unary implicant #2")
  {
    Var z1(solver, Var::Type::Binary, "z1");
    Var z2(solver, Var::Type::Binary, "z2");
    Var x(solver, Var::Type::Integer, std::nullopt, 2, "x");
    REQUIRE(((-z1 - z2 == -2) >> (x <= 0)).has_reformulation());
    auto const r = ((-z1 - z2 == -2) >> (x <= 0)).reformulation();
    REQUIRE(r.size() == 1);
    REQUIRE(fmt::format("{}", r[0]) == "x + 2 z1 + 2 z2 - 4 <= 0");
  }

  SECTION("Equality reformulation of non unary implicant")
  {
    Var z1(solver, Var::Type::Binary, "z1");
    Var z2(solver, Var::Type::Binary, "z2");
    Var x(solver, Var::Type::Integer, 2, 4, "x");
    REQUIRE(((z1 + z2 == 2) >> (x == 3)).has_reformulation());
    auto const r = ((z1 + z2 == 2) >> (x == 3)).reformulation();
    REQUIRE(r.size() == 2);
    REQUIRE(fmt::format("{}", r[0]) == "x + z1 + z2 - 5 <= 0");
    REQUIRE(fmt::format("{}", r[1]) == "-x + z1 + z2 + 1 <= 0");
  }
}

