lib_execution := libexecution.so
CC := gcc
CXX := g++
LOADER_FILES := Loader/PE.ldr.cpp Loader/Inproc.Mapper.cpp
EXTERNAL_FILES := ../revtracer/sync.cpp ../revtracer/crc32.cpp ../revtracer/cb.cpp
CPP_FILES := $(LOADER_FILES) LargeStack.cpp CommonExecutionController2.cpp InprocessExecutionController.cpp CommonExecutionController.cpp Main.cpp $(EXTERNAL_FILES)
OBJ_FILES := $(addprefix ,$(CPP_FILES:.cpp=.o))
LD_FLAGS :=
CC_FLAGS_CROSS = -D__cdecl="" -D__stdcall=""
CC_FLAGS += -g -m32 -std=c++11 -D_EXECUTION_EXPORTS -DBLOCK_CACHE_READ_ONLY -DINPROCESS_EXECUTION_ONLY $(CC_FLAGS_CROSS)
prefix := /usr/local

all: $(lib_execution)

install: $(lib_execution)
	install -m 0755 $(lib_execution) -t $(prefix)/lib -D

$(lib_execution): $(OBJ_FILES)
	$(CXX) $(CC_FLAGS) $(LD_FLAGS) -shared -o $@ $^

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_execution) $(OBJ_FILES)
