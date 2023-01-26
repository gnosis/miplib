This repo provides a common C++ interface on top of popular MIP solvers (backends).

Currently it supports:

* [Gurobi](https://www.gurobi.com/)
* [SCIP](https://www.scipopt.org/)
* [Lpsolve](http://lpsolve.sourceforge.net/5.5/)

The library requires at least one of these backends to be installed.

# Features

* Binary, Integer, and Continuous variables (to do: SOS).
* Linear constraints and objectives. 
* Quadratic constraints and objectives when supported by backend.
* Indicator constraints with automatic reformulation if not supported by backend.

# Example

```c++
using namespace miplib;

Solver solver(Solver::Backend::Scip);
solver.set_verbose(true);

Var v1(solver, Var::Type::Binary, "v1");
Var v2(solver, Var::Type::Binary, "v2");
Var v3(solver, Var::Type::Continuous, "v3");

solver.add(v1 == 1);
solver.add(v2 <= v1 - 1);
solver.add(v3 == v1 + v2);

auto r = solver.solve();

assert(r == Solver::Result::Optimal);

std::cout << v1.value() << std::endl; // prints 1
std::cout << v2.value() << std::endl; // prints 0
std::cout << v3.value() << std::endl; // prints 1
```

# Building

## Configure build (only required to do once)

```bash
[GUROBI=path-to-gurobi SCIP=path-to-scip] ./configure.sh
```

where GUROBI and SCIP vars might be required in case they are installed
in non-standard locations.

NOTE: Both paths point to path containing the `include` and `lib`dirs. For
example `path-to-gurobi` should be something like `/opt/gurobi903/linux64/`.

## Compile/Link

Either 

```bash
cmake --build build/{release|debug}
```

or

```bash
cd build/{release|debug}
make
```

## Run tests

```bash
./build/{release|debug}/test/unit_test
```
