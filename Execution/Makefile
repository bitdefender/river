lib_execution := libexecution.so
CC := gcc
CXX := g++

EXTERNAL_FILES := ../revtracer/sync.cpp ../revtracer/crc32.cpp ../revtracer/cb.cpp ../libproc/libproc.cpp

INCLUDE_PATHS := -I../BinLoader

INPROCESS_EXECUTION_CPP := LargeStack.cpp CommonExecutionController2.cpp InprocessExecutionController.cpp CommonExecutionController.cpp Main.cpp
EXTERN_EXECUTION_CPP := Debugger.cpp ExternExecutionController.Linux.cpp DualAllocator.Linux.cpp TokenRingInit.Linux.cpp

CPP_FILES := $(LOADER_FILES) $(EXTERN_EXECUTION_CPP) $(INPROCESS_EXECUTION_CPP) $(EXTERNAL_FILES)

OBJ_FILES := $(addprefix ,$(CPP_FILES:.cpp=.o))
LIB_BINLOADER_PATH := ../BinLoader/libbinloader.a
LIB_WRAPPER_SETUP_PATH := ../wrapper.setup/libwrappersetup.a
LIB_VIRTUALMEMORY_PATH := ../VirtualMemory/libvirtualmemory.a
LD_FLAGS := -lrt

CC_FLAGS_CROSS = -D__cdecl="" -D__stdcall=""
CC_FLAGS += -g -m32 -std=c++11 -D_EXECUTION_EXPORTS \
						-DBLOCK_CACHE_READ_ONLY  \
						$(INCLUDE_PATHS) $(CC_FLAGS_CROSS)

prefix := /usr/local

all: $(lib_execution)

install: $(lib_execution)
	install -m 0755 $(lib_execution) -t $(prefix)/lib -D

$(lib_execution): $(OBJ_FILES)
	$(CXX) $(CC_FLAGS)  -shared -o $@ $^ $(LIB_BINLOADER_PATH) $(LIB_WRAPPER_SETUP_PATH) $(LIB_VIRTUALMEMORY_PATH) $(LD_FLAGS)

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_execution) $(OBJ_FILES)
