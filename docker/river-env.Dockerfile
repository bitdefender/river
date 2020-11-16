FROM ubuntu:16.04

RUN  apt-get update && \
  apt-get upgrade -y && \
  apt-get install -y git wget sudo build-essential unzip && \
  rm -rf /var/lib/apt/lists/* 

# Cmake
RUN wget https://cmake.org/files/v3.14/cmake-3.14.0.tar.gz && \
  tar xzf cmake-3.14.0.tar.gz && \
  cd cmake-3.14.0 && \
  ./configure && \
  make && \
  make install

# Setup Triton

# Install Triton dependencies
# python3.6
RUN apt-get update && \
  apt-get install -y software-properties-common && \
  add-apt-repository ppa:deadsnakes/ppa && \
  apt-get update && \
  apt-get install -y python3.6 python3.6-dev python3-pip && \
  python3.6 -m pip install pip --upgrade && \
  python3.6 -m pip install wheel && \
  # boost
  apt-get install -y libboost-all-dev && \
  # z3
  wget https://github.com/Z3Prover/z3/archive/z3-4.8.1.zip && \
  unzip z3-4.8.1.zip && \
  rm -f z3-4.8.1.zip && \
  cd z3-z3-4.8.1 && \
  python3.6 scripts/mk_make.py && \
  cd build && \
  make && \
  make install && \
  # capstone
  wget https://github.com/aquynh/capstone/archive/4.0.2.zip && \
  unzip 4.0.2.zip && \
  rm -f 4.0.2.zip && ls && \
  cd capstone-4.0.2 && \
  chmod +x ./make.sh && \
  ./make.sh && \
  ./make.sh install && \
  # Install Triton
  git clone https://github.com/JonathanSalwan/Triton.git && \
  cd Triton && \
  mkdir build && \
  cd build && \
  cmake .. && \
  make -j2 install

RUN wget https://github.com/aquynh/capstone/archive/4.0.2.zip && \
  unzip 4.0.2.zip && \
  rm -f 4.0.2.zip && ls && \
  cd capstone-4.0.2 && \
  chmod +x ./make.sh && \
  ./make.sh && \
  ./make.sh install 

# Install LIEF
RUN python3.6 -m pip install setuptools --upgrade && \
  pip install lief