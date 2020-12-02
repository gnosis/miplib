# A container with the 3 backends supported at the moment (Gurobi, SCIP and lpsolve) installed.

# NOTE: only the binary libs are installed (e.g. no python support).
# NOTE: some of these backends still require an installed license to operate.

FROM ubuntu:bionic as COMPILE

RUN apt-get update \
  && apt-get install -y --no-install-recommends wget libblas3 libgfortran4 libgsl23 libgslcblas0 liblapack3 libtbb2 libboost-program-options1.65.1 lp-solve \
  && rm -rf /var/lib/apt/lists/*

# Install SCIP solver
WORKDIR /opt
RUN wget https://scip.zib.de/download/release/SCIPOptSuite-7.0.1-Linux.deb --no-check-certificate  \
  && (dpkg -i SCIPOptSuite-7.0.1-Linux.deb || sudo apt-get install -f)

ENV SCIP_DIR="/usr"

# Install Gurobi solver
RUN wget --no-check-certificate https://packages.gurobi.com/9.1/gurobi9.1.0_linux64.tar.gz \
  && tar xzf gurobi9.1.0_linux64.tar.gz

ENV GUROBI_DIR="/opt/gurobi910/linux64"

