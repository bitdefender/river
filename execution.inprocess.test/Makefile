mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
makefile_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

inprocess_test := inprocess_test
disable_sse_so := libdisablesse.so
CC := gcc
CXX := g++
CPP_FILES := Main.cpp
OBJ_FILES := Main.o
LD_PATHS:= -L$(makefile_dir)/../Execution -L$(makefile_dir)/../ParserPayload -L$(makefile_dir)
LD_FLAGS:= -lexecution -lpthread -lhttp-parser -ldisablesse -ldl
CC_FLAGS_CROSS = -D__cdecl="" -D__stdcall=""
NO_SSE := -mno-mmx -mno-sse -march=i386
CXX_FLAGS += -m32 $(NO_SSE) -std=c++11 $(CC_FLAGS_CROSS)
CC_FLAGS += -m32 $(NO_SSE) $(CC_FLAGS_CROSS)
prefix := /usr/local

ASM_FILES := DisableSSE.S
C_FILES := DisableSSEHelper.c
OBJ_FILES_SO :=  DisableSSE.o  DisableSSEHelper.o

all: $(inprocess_test)

install: $(inprocess_test)
	install -m 0755 $(inprocess_test) -t $(prefix)/bin -D
	install -m 0755 $(disable_sse_so) -t $(prefix)/lib -D

$(inprocess_test): $(OBJ_FILES) $(disable_sse_so)
	$(CXX) $(LD_PATHS) $(CXX_FLAGS) -o $@ $(OBJ_FILES) $(LD_FLAGS)

$(disable_sse_so): $(OBJ_FILES_SO)
	$(CC) $(CC_FLAGS) -shared -o $@ $^

Main.o: Main.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

DisableSSEHelper.o: DisableSSEHelper.c
	$(CC) $(CC_FLAGS) -c -o $@ $<

DisableSSE.o: DisableSSE.S
	$(CC) -m32 -march=i386 -I./include -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(inprocess_test) $(disable_sse_so) $(OBJ_FILES_SO) $(OBJ_FILES)
