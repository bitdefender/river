ARG parent_image
FROM $parent_image  

RUN  apt-get update && \
  apt-get upgrade -y && \
  apt-get install -y git sudo build-essential unzip libssl-dev wget && \
  rm -rf /var/lib/apt/lists/* 

# Setup Triton
RUN mkdir /river

# python3.6
RUN apt-get update && \
  apt-get install -y software-properties-common && \
  add-apt-repository ppa:deadsnakes/ppa && \
  apt-get update && \
  apt-get install -y python3.6 python3.6-dev python3-pip && \
  python3.6 -m pip install pip --upgrade && \
  python3.6 -m pip install wheel

# boost
RUN apt-get install -y libboost-all-dev

# cmake
RUN cd /river && \
  wget https://github.com/Kitware/CMake/releases/download/v3.19.1/cmake-3.19.1-Linux-x86_64.tar.gz && \
  tar -zxvf cmake-3.19.1-Linux-x86_64.tar.gz && \
  ln -sf /river/cmake-3.19.1-Linux-x86_64/bin/* /usr/local/bin/

# capstone
RUN cd /river && \
  wget https://github.com/aquynh/capstone/archive/4.0.2.zip && \
  unzip 4.0.2.zip && \
  rm -f 4.0.2.zip && \
  cd capstone-4.0.2 && \
  chmod +x ./make.sh && \
  ./make.sh && \
  ./make.sh install

# z3
RUN cd /river && \
  wget https://github.com/Z3Prover/z3/releases/download/z3-4.8.9/z3-4.8.9-x64-ubuntu-16.04.zip && \
  unzip z3-4.8.9-x64-ubuntu-16.04.zip && \
  ln -sf /river/z3-4.8.9-x64-ubuntu-16.04/bin/* /usr/local/bin/ && \
  ln -sf /river/z3-4.8.9-x64-ubuntu-16.04/include/* /usr/local/include/

# Install Triton
RUN cd /river && \
  git clone https://github.com/JonathanSalwan/Triton.git && \
  cd Triton && \
  mkdir build && \
  cd build && \
  cmake .. && \
  make -j2 install

# Install LIEF
RUN python3.6 -m pip install setuptools --upgrade && \
  pip install lief

RUN pip install --use-feature=2020-resolver numpy tensorflow
 
# Clone river code
RUN git clone https://github.com/unibuc-cs/river.git /river/repo

# Use empty libfuzzer as honggfuzz
RUN cd /river && \
  echo "int main(){}" > empty_lib.c && \  
  cc -c -o empty_lib.o empty_lib.c

# ENTRYPOINT [ "python3.6", "python/concolic_RLGenerationalSearch.py"]
# CMD ["--secondsBetweenStats", "2", "--architecture", "x64", "--maxLen", "1", "--outputType", "textual"]

