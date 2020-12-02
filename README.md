This repo provides a common C++ interface on top of popular MIP solvers (backends).

Currently it supports:

* [Gurobi] (https://www.gurobi.com/)
* [SCIP] (https://www.scipopt.org/)
* [Lpsolve] (http://lpsolve.sourceforge.net/5.5/)

The library requires at least one of these backends to be installed.

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
