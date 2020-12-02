#!/bin/bash

cmake -H. -Bbuild/release -DCMAKE_BUILD_TYPE=Release -DGUROBI:PATH=$GUROBI -DSCIP:PATH=$SCIP
cmake -H. -Bbuild/debug -DCMAKE_BUILD_TYPE=Debug -DGUROBI:PATH=$GUROBI -DSCIP:PATH=$SCIP
