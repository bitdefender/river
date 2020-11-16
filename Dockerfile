FROM cconache/river3-env

COPY ["./River3", "/River3"]
WORKDIR /River3

ENTRYPOINT [ "/bin/bash" ]