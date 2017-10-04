mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
makefile_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

libjpeg_turbo := libjpeg_turbo.so
CC := gcc
CXX := g++
INCLUDE_PATHS = -I$(makefile_dir)/libjpeg-turbo-src
OBJ_FILES := Payload.o $(makefile_dir)/libjpeg-turbo-src/.libs/libturbojpeg.a
CXX_FLAGS += -m32 -std=c++11 -D_PAYLOAD_EXPORTS
prefix := /usr/local

all: $(libjpeg_turbo)

install: $(libjpeg_turbo)
	install -m 0755 $(libjpeg_turbo) -t $(prefix)/lib -D

$(libjpeg_turbo): $(OBJ_FILES)
	$(CXX) -m32 -shared $(OBJ_FILES) -o $@

Payload.o: Payload.cpp
	   $(CXX) $(CXX_FLAGS) $(INCLUDE_PATHS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(libjpeg_turbo) Payload.o
