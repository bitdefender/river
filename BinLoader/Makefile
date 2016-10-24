test_bin := test_bin
lib_binloader := libbinloader.so
CC := gcc
CXX := g++
LOADER_FILES := ELF.Loader.cpp PE.Loader.cpp Unified.Loader.cpp
MAPPER_FILES := Inproc.Mapper.cpp
IMPORTER_FILES := Inproc.Native.Importer.cpp
CPP_FILES := Common.cpp $(LOADER_FILES) $(MAPPER_FILES) $(IMPORTER_FILES)
OBJ_FILES := $(addprefix ,$(CPP_FILES:.cpp=.o))
LD_FLAGS := -ldl
CXX_FLAGS += -m32 -std=c++11
prefix := /usr/local

all: $(lib_binloader)

test_bin: $(lib_binloader) Main.o
	$(CXX) -L. $(CXX_FLAGS) -o main Main.o -lbinloader

install: $(lib_binloader)
	install -m 0755 $(lib_binloader) -t $(prefix)/lib -D

$(lib_binloader): $(OBJ_FILES)
	$(CXX) $(CXX_FLAGS) -shared -o $@ $^ $(LD_FLAGS)

%.o: %.cpp
	   $(CXX) $(CXX_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_binloader) $(OBJ_FILES)
