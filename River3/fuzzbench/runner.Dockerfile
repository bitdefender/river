FROM gcr.io/fuzzbench/base-image

RUN  apt-get update && \
  apt-get upgrade -y && \
  apt-get dist-upgrade -y
