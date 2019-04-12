mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
makefile_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

lib_gumbo := libgumbo.so
CC := gcc
CXX := g++
INCLUDE_PATHS = -I$(makefile_dir)/gumbo-src/src
OBJ_FILES := Payload.o $(makefile_dir)/gumbo-src/.libs/libgumbo.a
CXX_FLAGS += -m32 -D_PAYLOAD_EXPORTS
prefix := /usr/local

all: $(lib_gumbo)

install: $(lib_gumbo)
	install -m 0755 $(lib_gumbo) -t $(prefix)/lib -D

$(lib_gumbo): $(OBJ_FILES)
	$(CXX) -m32 -shared $(OBJ_FILES) -o $@

Payload.o: Payload.cpp
	   $(CXX) $(CXX_FLAGS) $(INCLUDE_PATHS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_gumbo) Payload.o
