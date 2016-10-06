lib_ipc := ipclib.dll
CC := i686-w64-mingw32-gcc
CXX := i686-w64-mingw32-g++
CPP_FILES := ipclib.cpp ShmTokenRing.cpp
OBJ_FILES := $(addprefix ,$(notdir $(CPP_FILES:.cpp=.o)))
LD_FLAGS :=
CC_FLAGS_CROSS =-D_cdecl="" -D_stdcall=""
CC_FLAGS += -m32 -std=c++11 -fcheck-new -D_BUILDING_IPC_DLL $(CC_FLAGS_CROSS)

all: $(lib_ipc)

$(lib_ipc): $(OBJ_FILES)
	$(CXX) -shared -o $@ \
		-Wl,--export-all-symbols \
		-Wl,--enable-auto-import $^

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_ipc) $(OBJ_FILES)
