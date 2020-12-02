# A container with the 3 backends supported at the moment (Gurobi, SCIP and lpsolve) installed.
# Also includes cmake and a c++ compiler to build miplib.

# NOTE: only the binary libs are installed (e.g. no python support).
# NOTE: some of these backends still require an installed license to operate.

FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    wget libblas3 libgfortran4 libgsl23 libgslcblas0 liblapack3 libtbb2 libboost-program-options1.71 lp-solve \
    build-essential cmake g++-9 libgmp-dev libblas-dev libbliss-dev zlib1g-dev libgsl-dev libcliquer-dev \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /opt

# Install SCIP solver
RUN wget https://scip.zib.de/download/release/scipoptsuite-7.0.1.tgz --no-check-certificate  \
  && tar -xzvf scipoptsuite-7.0.1.tgz \
  && cd scipoptsuite-7.0.1 \
  && mkdir build \
  && cd build \
  && cmake .. \
  && make \
  && make install \
  && cd ../.. \
  && rm -fr scipoptsuite-7.0.1

# Install Gurobi solver
RUN wget --no-check-certificate https://packages.gurobi.com/9.1/gurobi9.1.0_linux64.tar.gz \
  && tar xzf gurobi9.1.0_linux64.tar.gz

ENV GUROBI_DIR="/opt/gurobi910/linux64"

# TODO: install this package in the beginning like the others
RUN apt-get update \
  && apt-get install -y --no-install-recommends git ca-certificates ssh liblpsolve55-dev
