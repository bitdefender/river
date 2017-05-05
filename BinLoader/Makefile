test_bin := test_bin
lib_binloader := libbinloader.a
CC := gcc
CXX := g++
LOADER_FILES := ELF.Loader.cpp PE.Loader.cpp Unified.Loader.cpp LoaderAPI.cpp
MAPPER_FILES := Inproc.Mapper.cpp Shm.Mapper.cpp
IMPORTER_FILES := Inproc.Native.Importer.cpp
CPP_FILES := Common.cpp $(LOADER_FILES) $(MAPPER_FILES) $(IMPORTER_FILES)
OBJ_FILES := $(addprefix ,$(CPP_FILES:.cpp=.o))
CXX_FLAGS += -g -m32 -std=c++11
prefix := /usr/local

all: $(lib_binloader)

test_bin: $(lib_binloader) Main.o
	$(CXX) -L. $(CXX_FLAGS) -o main Main.o -lbinloader -ldl

install: $(lib_binloader)
	install -m 0755 $(lib_binloader) -t $(prefix)/lib -D

$(lib_binloader): $(OBJ_FILES)
	$(AR) rcs $@ $^

%.o: %.cpp
	   $(CXX) $(CXX_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_binloader) $(OBJ_FILES) *.o main
