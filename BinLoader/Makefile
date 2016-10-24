lib_binloader := libbinloader
CC := gcc
CXX := g++
LOADER_FILES := ELF.Loader.cpp PE.Loader.cpp Unified.Loader.cpp
MAPPER_FILES := Inproc.Mapper.cpp
IMPORTER_FILES := Inproc.Native.Importer.cpp
CPP_FILES := Common.cpp Main.cpp $(LOADER_FILES) $(MAPPER_FILES) $(IMPORTER_FILES)
OBJ_FILES := $(addprefix ,$(CPP_FILES:.cpp=.o))
LD_FLAGS :=
CC_FLAGS_CROSS = -D__cdecl="" -D__stdcall=""
CC_FLAGS += -g -m32 -march=i386 -fstack-protector -std=c++11 \
							$(CC_FLAGS_CROSS)
prefix := /usr/local

all: $(lib_binloader)

install: $(lib_binloader)
	install -m 0755 $(lib_binloader) -t $(prefix)/lib -D

$(lib_binloader): $(OBJ_FILES)
	$(CXX) $(CC_FLAGS) $(LD_FLAGS) -shared -o $@ $^

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_binloader) $(OBJ_FILES)
