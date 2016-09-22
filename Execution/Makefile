lib_execution := execution.so
CC := gcc
CXX := g++
CPP_FILES := ../ipclib/ShmTokenRing.cpp ../revtracer/cb.cpp ../revtracer/crc32.cpp ../revtracer/sync.cpp CommonExecutionController.cpp DualAllocator.cpp ExternExecutionController.cpp InprocessExecutionController.cpp CommonExecutionController2.cpp LargeStack.cpp Loader/Extern.Mapper.cpp Loader/Inproc.Mapper.cpp Loader/Mem.Mapper.cpp Loader/PE.ldr.cpp Main.cpp
OBJ_FILES := $(addprefix ,$(CPP_FILES:.cpp=.o))
LD_FLAGS :=
CC_FLAGS_CROSS = -D_cdecl="" -D__stdcall=""
CC_FLAGS += -m32 -std=c++11 -D_EXECUTION_EXPORTS $(CC_FLAGS_CROSS)

all: $(lib_execution)

$(lib_execution): $(OBJ_FILES)
	$(CXX) -shared -o $@ /
		-Wl,--export-all-symbols /
		-Wl,--enable-auto-import $^

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_execution) $(OBJ_FILES)

CC_FLAGS += -MMD
-include $(OBJFILES:.o=.d)
