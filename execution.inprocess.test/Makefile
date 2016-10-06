mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
makefile_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

inprocess_test := inprocess_test
CC := gcc
CXX := g++
CPP_FILES := Main.cpp Payload.cpp
OBJ_FILES := $(addprefix ,$(CPP_FILES:.cpp=.o))
LD_PATHS:= -L$(makefile_dir)/../Execution
LD_FLAGS:= -lexecution -lpthread -ldl
CC_FLAGS_CROSS = -D__cdecl="" -D__stdcall=""
CC_FLAGS += -g -m32 -O0 -fno-stack-protector -std=c++11 $(CC_FLAGS_CROSS)
prefix := /usr/local

all: $(inprocess_test)

install: $(inprocess_test)
	install -m 0755 $(inprocess_test) -t $(prefix)/bin -D

$(inprocess_test): $(OBJ_FILES)
	$(CXX) $(LD_PATHS) $(CC_FLAGS) -o $@ $^ $(LD_FLAGS)

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(inprocess_test) $(OBJ_FILES)
