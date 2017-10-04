mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
makefile_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

libpng := libpng.so
CC := gcc
CXX := g++
INCLUDE_PATHS = -I$(makefile_dir)/libpng-src/src
OBJ_FILES := Payload.o $(makefile_dir)/libpng-src/.libs/libpng16.a
CXX_FLAGS += -m32 -std=c++11 -D_PAYLOAD_EXPORTS
prefix := /usr/local

all: $(libpng)

install: $(libpng)
	install -m 0755 $(libpng) -t $(prefix)/lib -D

$(libpng): $(OBJ_FILES)
	$(CXX) -m32 -shared $(OBJ_FILES) -o $@ -lz

Payload.o: Payload.cpp
	   $(CXX) $(CXX_FLAGS) $(INCLUDE_PATHS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(libpng) Payload.o
