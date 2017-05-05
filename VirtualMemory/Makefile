lib_virtualmemory := libvirtualmemory.a
CC := gcc
CXX := g++

EXTERNAL_FILES := ../libproc/libproc.cpp
CPP_FILES := VirtualMem.cpp MemoryLayout.Linux.cpp $(EXTERNAL_FILES)

OBJ_FILES := $(addprefix ,$(CPP_FILES:.cpp=.o))
CXX_FLAGS += -g -m32 -std=c++11
prefix := /usr/local

all: $(lib_virtualmemory)

test_bin: $(lib_virtualmemory) Main.o
	$(CXX) -L. $(CXX_FLAGS) -o main Main.o -lbinloader -ldl

install: $(lib_virtualmemory)
	install -m 0755 $(lib_virtualmemory) -t $(prefix)/lib -D

$(lib_virtualmemory): $(OBJ_FILES)
	$(AR) rcs $@ $^

%.o: %.cpp
	   $(CXX) $(CXX_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_virtualmemory) $(OBJ_FILES) *.o main
