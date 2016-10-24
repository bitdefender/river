mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
makefile_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

libxml := libxml.so
CC := gcc
CXX := g++
LIBPATHS := -L$(makefile_dir) -L$(makefile_dir)/build/.libs/
OBJ_FILES :=  Payload.o
INCLUDE_PATH := -I$(makefile_dir)/build/include -I$(makefile_dir)/libxml2-src/include
NO_SSE := -mno-mmx -mno-sse -march=i386
CXX_FLAGS += -m32 -std=c++11 $(NO_SSE) -D_PAYLOAD_EXPORTS
prefix := /usr/local

all: $(libxml)

install: $(libxml)
	install -m 0755 $(libxml) -t $(prefix)/lib -D

$(libxml): $(OBJ_FILES)
	$(CXX) -m32 -shared $(LIBPATHS) $(OBJ_FILES) -lxml2 -o $@

Payload.o: Payload.cpp
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) -r $(libxml) Payload.o #build/
