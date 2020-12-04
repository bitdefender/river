FROM circleci/buildpack-deps:20.04
ARG VERSION=unknown

LABEL maintainer="eduard.c.staniloiu@gmail.com" \
      name="UB UPB River Implementation" \
      version="${VERSION}"

WORKDIR /home/circleci

ENV DEBIAN_FRONTEND="noninteractive"

# Install missing packages and required dependencies
RUN sudo apt-get update && \
    sudo apt-get install -y wget vim ssh git python3 python3-pip \
        libboost-all-dev libz3-dev cmake

# Use pip to install latest capstone and lief libraries
RUN pip3 install setuptools --upgrade && \
    pip3 install capstone lief tensorflow

# Clone River and it's submodules
RUN git clone --recursive https://github.com/AGAPIA/river.git

# Build Triton from source and install it in the default paths
# Will extract the path to where pip installes packages so it can tell Triton where to find libcapstone
# Triton expects the library name to be libcapstone.so.4, so it will also create a symlink with this name
# This RUN command has to be invoked through `/bin/bash -c ...` so it can expand the PYTHON_PACKAGES var
RUN ["/bin/bash", "-c", "cd /home/circleci/river/River3/ExternalTools/Triton && mkdir build && cd build && \
    PYTHON_PACKAGES=$(pip3 show capstone | grep Location | cut -f 2 -d ' ') && \
    CAPSTONE_INCLUDE_DIRS=${PYTHON_PACKAGES}/capstone/include/ \
    CAPSTONE_LIBRARIES=${PYTHON_PACKAGES}/capstone/lib/libcapstone.so \
    cmake .. && \
    make -j2 && \
    sudo make install && \
    sudo ln -s ${PYTHON_PACKAGES}/capstone/lib/libcapstone.so ${PYTHON_PACKAGES}/capstone/lib/libcapstone.so.4"]

# Run example test when container starts
CMD cd /home/circleci/river/River3/TestPrograms && \
    gcc -g -O0 -o crackme_xor crackme_xor.c && \
    cd ../python && \
    python3 concolic_GenerationalSearch2.py --binaryPath "../TestPrograms/crackme_xor" \
                                            --architecture x64 \
                                            --maxLen 1 \
                                            --targetAddress 0x11d3 \
                                            --logLevel CRITICAL \
                                            --secondsBetweenStats 10 \
                                            --outputType textual && \
    python3 concolic_RLGenerationalSearch.py --binaryPath "../TestPrograms/crackme_xor" \
                                             --architecture x64 \
                                             --maxLen 1 \
                                             --targetAddress 0x11d3 \
                                             --logLevel CRITICAL \
                                             --secondsBetweenStats 10 \
                                             --outputType textual

