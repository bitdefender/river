mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
makefile_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

libjsmn := libjsmn.so
CC := gcc
CXX := g++
###LIBPATHS := -L$(makefile_dir)
OBJ_FILES :=  Payload.o $(makefile_dir)/libjsmn-src/libjsmn.a
INCLUDE_PATH := -I$(makefile_dir)/libjsmn-src
CXX_FLAGS += -m32 -std=c++11 -D_PAYLOAD_EXPORTS
prefix := /usr/local

all: $(libjsmn)

install: $(libjsmn)
	install -m 0755 $(libjsmn) -t $(prefix)/lib -D

$(libjsmn): $(OBJ_FILES)
	$(CXX) -m32 -shared $(LIBPATHS) $(OBJ_FILES) -o $@

Payload.o: Payload.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) -r $(libjsmn) Payload.o
