# A container with the 3 backends supported at the moment (Gurobi, SCIP and lpsolve) installed.
# Also includes cmake and a c++ compiler to build miplib.

# NOTE: only the binary libs are installed (e.g. no python support).
# NOTE: some of these backends still require an installed license to operate.

# Required in order to use fast HSL code
# Install SCIP solver (from source)

FROM ubuntu:focal

ARG SCIP_VERSION=8.0.3

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update &&\
    apt-get install -y --no-install-recommends \
        build-essential ca-certificates cmake curl g++-9 git wget \
        libboost-program-options1.71 liblpsolve55-dev &&\
    curl -L -o scip.deb https://www.scipopt.org/download/release/SCIPOptSuite-${SCIP_VERSION}-Linux-ubuntu.deb &&\
    apt-get install -y ./scip.deb &&\
    rm scip.deb

WORKDIR /opt

# Install Gurobi solver
RUN wget --no-check-certificate https://packages.gurobi.com/9.5/gurobi9.5.0_linux64.tar.gz \
  && tar xzf gurobi9.5.0_linux64.tar.gz

ENV GUROBI_DIR="/opt/gurobi950/linux64"

RUN cd $GUROBI_DIR/src/build && \
  make && \
  mv libgurobi_c++.a ../../lib && \
  cd /opt

ENV LD_LIBRARY_PATH=/usr/local/lib
