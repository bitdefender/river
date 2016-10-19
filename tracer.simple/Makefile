mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
makefile_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

simple_tracer := simple_tracer
CC := gcc
CXX := g++
CPP_FILES := Main.cpp
OBJ_FILES := Main.o
LD_PATHS:= -L$(makefile_dir)/../DisableSSE -L$(makefile_dir)/../Execution -L$(makefile_dir)

LD_FLAGS:= -lexecution -lpthread -ldl -ldisablesse
CC_FLAGS_CROSS = -D__cdecl="" -D__stdcall=""
NO_SSE := -mno-mmx -mno-sse -march=i386
CXX_FLAGS += -m32 $(NO_SSE) -std=c++11 $(CC_FLAGS_CROSS)
CC_FLAGS += -m32 $(NO_SSE) $(CC_FLAGS_CROSS)
prefix := /usr/local

all: $(simple_tracer)

install: $(simple_tracer)
	install -m 0755 $(simple_tracer) -t $(prefix)/bin -D

$(simple_tracer): $(OBJ_FILES)
	$(CXX) $(LD_PATHS) $(CXX_FLAGS) -o $@ $(OBJ_FILES) $(LD_FLAGS)

Main.o: Main.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(simple_tracer) $(OBJ_FILES_SO) $(OBJ_FILES)
