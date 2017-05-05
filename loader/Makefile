mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
makefile_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

lib_loader := libloader.so
CC := gcc
CXX := g++
AS := as

CPP_FILES := Loader.Linux.cpp
AS_FILES := Loader.Linux.Stub.S
OBJ_FILES := Loader.Linux.o Loader.Linux.Stub.o 
CC_FLAGS += -m32 -std=c++11
AS_FLAGS += --32
LIB_BINLOADER_PATH := ../BinLoader/libbinloader.a
LD_FLAGS := -lrt -ldl -ldisablesse
LD_PATHS := -L$(makefile_dir)/../DisableSSE

prefix := /usr/local

all: $(lib_loader)

test: all
	g++ $(CC_FLAGS) -L. Main.cpp -o test -lloader

run-test: test
	LD_PRELOAD=$(lib_loader) ./test

install: $(lib_loader)
	install -m 0755 $(lib_loader) -t $(prefix)/lib -D

$(lib_loader): $(OBJ_FILES)
	$(CXX) $(CC_FLAGS) $(LD_PATHS) -shared -o $@ $^ $(LIB_BINLOADER_PATH) $(LD_FLAGS)

%.o: %.cpp
	$(CXX) $(CC_FLAGS) -c -o $@ $<

%.o: %.S
	$(AS) $(AS_FLAGS) -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_loader) $(OBJ_FILES) test
