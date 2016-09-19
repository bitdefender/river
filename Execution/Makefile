lib_execution := execution.so
CC := gcc
CXX := g++
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix ,$(notdir $(CPP_FILES:.cpp=.o)))
LD_FLAGS :=
CC_FLAGS_CROSS = -D_cdecl="" -D__stdcall=""
CC_FLAGS += -m32 -std=c++11 $(CC_FLAGS_CROSS)

all: $(lib_execution)

$(lib_execution): $(OBJ_FILES)
	$(CXX) -shared -o $@ \
		-Wl,--export-all-symbols \
		-Wl,--enable-auto-import $^

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_execution) $(OBJ_FILES)

CC_FLAGS += -MMD
-include $(OBJFILES:.o=.d)
