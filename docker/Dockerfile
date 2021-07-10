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
    libreadline-dev git ca-certificates ssh liblpsolve55-dev \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /opt

# Install SCIP solver
RUN wget https://scip.zib.de/download/release/SCIPOptSuite-7.0.2-Linux-ubuntu.sh --no-check-certificate && \
  chmod +x ./SCIPOptSuite-7.0.2-Linux-ubuntu.sh && \
 ./SCIPOptSuite-7.0.2-Linux-ubuntu.sh --skip-license

# Install Gurobi solver
RUN wget --no-check-certificate https://packages.gurobi.com/9.1/gurobi9.1.1_linux64.tar.gz \
  && tar xzf gurobi9.1.1_linux64.tar.gz

ENV GUROBI_DIR="/opt/gurobi911/linux64"

RUN cd $GUROBI_DIR/src/build && \
  make && \
  mv libgurobi_c++.a ../../lib && \
  cd /opt
