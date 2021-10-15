# A container with the 3 backends supported at the moment (Gurobi, SCIP and lpsolve) installed.
# Also includes cmake and a c++ compiler to build miplib.

# NOTE: only the binary libs are installed (e.g. no python support).
# NOTE: some of these backends still require an installed license to operate.


# Required in order to use fast HSL code
# Install SCIP solver (from source)

ARG SCIP_VERSION=7.0.2
ARG IPOPT_VERSION=3.14.4

FROM ubuntu:latest as COMPILE

ARG SCIP_VERSION
ARG IPOPT_VERSION

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
  && apt-get install -y --no-install-recommends g++ gfortran make cmake ca-certificates wget   \
  pkg-config libblas3 libgfortran4 libgsl23 libgslcblas0 liblapack3 libtbb2 libboost-program-options1.71 \
    build-essential libgmp-dev libblas-dev libbliss-dev zlib1g-dev libgsl-dev libcliquer-dev \
    libreadline-dev liblapack-dev git file \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /opt

# Install hsl (IpOpt dependency)
COPY ./docker/coinhsl-2021.05.05.tar.gz $WORKDIR
RUN git clone https://github.com/coin-or-tools/ThirdParty-HSL.git \
  && cd ThirdParty-HSL \
  && tar xzvf ../coinhsl-2021.05.05.tar.gz \
  && mv coinhsl-2021.05.05 coinhsl \
  && ./configure \
  && make \
  && make install

# Install IpOpt from source (SCIP dependency)
RUN cd $WORKDIR
RUN wget https://github.com/coin-or/Ipopt/archive/refs/tags/releases/${IPOPT_VERSION}.tar.gz \
  && tar xzvf ${IPOPT_VERSION}.tar.gz \
  && cd Ipopt-releases-${IPOPT_VERSION} \
  && mkdir build \
  && cd build \
  && ADD_CFLAGS=-fopenmp ADD_FFLAGS=-fopenmp ADD_CXXFLAGS=-fopenmp ../configure --with-hsl-cflags="-I/usr/local/include/coin-or/hsl/" --with-hsl-lflags="-L/usr/local/lib/ -lcoinhsl" \
  && make \
  && make install

RUN cd $WORKDIR
RUN wget https://scip.zib.de/download/release/scipoptsuite-${SCIP_VERSION}.tgz --no-check-certificate  \
  && tar xzf scipoptsuite-${SCIP_VERSION}.tgz \
  && cd scipoptsuite-${SCIP_VERSION} \
  && mkdir build \
  && cd build \
  && cmake .. \
  && make CXX_FLAGS="-DSOPLEX_WITH_GMP -fPIC" install \
  && cd $WORKDIR

# Runtime image

FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    wget libblas3 libgfortran4 libgsl23 libgslcblas0 liblapack3 libtbb2 libboost-program-options1.71 lp-solve \
    build-essential cmake g++-9 libgmp-dev libblas-dev libbliss-dev zlib1g-dev libgsl-dev libcliquer-dev \
    libreadline-dev git ca-certificates ssh liblpsolve55-dev libgomp1 \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /opt

COPY --from=COMPILE /usr/local/lib/ /usr/local/lib/
COPY --from=COMPILE /usr/local/include/ /usr/local/include/

# Install SCIP solver (binaries)
#RUN wget https://scip.zib.de/download/release/SCIPOptSuite-7.0.2-Linux-ubuntu.sh --no-check-certificate && \
#  chmod +x ./SCIPOptSuite-7.0.2-Linux-ubuntu.sh && \
# ./SCIPOptSuite-7.0.2-Linux-ubuntu.sh --skip-license

# Install Gurobi solver
RUN wget --no-check-certificate https://packages.gurobi.com/9.1/gurobi9.1.1_linux64.tar.gz \
  && tar xzf gurobi9.1.1_linux64.tar.gz

ENV GUROBI_DIR="/opt/gurobi911/linux64"

RUN cd $GUROBI_DIR/src/build && \
  make && \
  mv libgurobi_c++.a ../../lib && \
  cd /opt

ENV LD_LIBRARY_PATH=/usr/local/lib