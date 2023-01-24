# A container with the 3 backends supported at the moment (Gurobi, SCIP and lpsolve) installed.
# Also includes cmake and a c++ compiler to build miplib.

# NOTE: only the binary libs are installed (e.g. no python support).
# NOTE: some of these backends still require an installed license to operate.

ARG SCIP_VERSION=8.0.3
ARG IPOPT_VERSION=3.14.4

FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    libblas3 libgfortran4 libgsl23 libgslcblas0 liblapack3 libtbb2 libboost-program-options1.71 lp-solve \
    build-essential cmake g++-9 libgmp-dev libblas-dev libbliss-dev zlib1g-dev libgsl-dev libcliquer-dev \
    libreadline-dev git ca-certificates ssh liblpsolve55-dev libgomp1 curl patchelf \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /opt

 # Install SCIP solver (binaries)
RUN curl -L https://scip.zib.de/download/release/SCIPOptSuite-8.0.3-Linux-ubuntu.sh --output installer.sh && \
  chmod +x ./installer.sh && \
 ./installer.sh --skip-license && \
 rm installer.sh

# Install Gurobi solver
RUN curl --no-check-certificate https://packages.gurobi.com/9.5/gurobi9.5.1_linux64.tar.gz --output gurobi.tar.gz \
  && tar xzf gurobi.tar.gz \
  && rm gurobi.tar.gz

ENV GUROBI="/opt/gurobi951/linux64"

# Fix ABI incompatibilities of the pre compiled gurobi lib.
# See https://support.gurobi.com/hc/en-us/articles/360039093112-How-do-I-resolve-undefined-reference-errors-while-linking-Gurobi-in-C-
RUN cd $GUROBI/src/build && \
  make && \
  mv libgurobi_c++.a ../../lib && \
  cd /opt

ENV LD_LIBRARY_PATH=/usr/local/lib:$GUROBI/lib
