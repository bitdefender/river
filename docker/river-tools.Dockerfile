FROM cconache/river-env

ENV HOME ~
ENV LD_LIBRARY_PATH=/usr/local/lib/

WORKDIR $HOME

# Install River and tracer tools

# Download and unzip prebuilt-Z3 libraries
RUN wget https://storage.googleapis.com/river-artifacts/z3.tar.gz && \
  tar -xzf z3.tar.gz && \
  rm z3.tar.gz && \
  # Download River Tools install script
  wget https://raw.githubusercontent.com/AGAPIA/river/master/River2/installRiverTools.sh && \
  # Run install script
  mkdir testtools && \
  chmod +x ./installRiverTools.sh && \
  ./installRiverTools.sh testtools Debug