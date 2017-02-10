lib_execution := libexecution.so
CC := gcc
CXX := g++
EXTERNAL_FILES := ../revtracer/sync.cpp ../revtracer/crc32.cpp ../revtracer/cb.cpp
INCLUDE_PATHS := -I../BinLoader
CPP_FILES := $(LOADER_FILES) LargeStack.cpp CommonExecutionController2.cpp InprocessExecutionController.cpp CommonExecutionController.cpp Main.cpp $(EXTERNAL_FILES)
OBJ_FILES := $(addprefix ,$(CPP_FILES:.cpp=.o))
LIB_BINLOADER_PATH := ../BinLoader/libbinloader.a
CC_FLAGS_CROSS = -D__cdecl="" -D__stdcall=""
CC_FLAGS += -g -m32 -march=i386 -std=c++11 -D_EXECUTION_EXPORTS \
						-DBLOCK_CACHE_READ_ONLY -DINPROCESS_EXECUTION_ONLY \
						$(INCLUDE_PATHS) $(CC_FLAGS_CROSS)
LD_FLAGS := -ldl
prefix := /usr/local

all: $(lib_execution)

install: $(lib_execution)
	install -m 0755 $(lib_execution) -t $(prefix)/lib -D

$(lib_execution): $(OBJ_FILES)
	$(CXX) $(CC_FLAGS) -shared -o $@ $^ $(LIB_BINLOADER_PATH) $(LD_FLAGS)

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_execution) $(OBJ_FILES)
