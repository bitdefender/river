FROM cconache/river3-env

RUN pip install --use-feature=2020-resolver numpy tensorflow

# The docker context must be the River3 directory
COPY [".", "/River3"]
WORKDIR /River3

ENTRYPOINT [ "/bin/bash" ]