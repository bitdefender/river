inprocess_test := inprocess_test
CC := gcc
CXX := g++
CPP_FILES := Main.cpp Payload.cpp
OBJ_FILES := $(addprefix ,$(CPP_FILES:.cpp=.o))
LD_PATHS:= -L$(shell pwd)/../Execution
LD_FLAGS:= -lexecution -lpthread -ldl
CC_FLAGS_CROSS = -D__cdecl="" -D__stdcall=""
CC_FLAGS += -m32 -std=c++11 $(CC_FLAGS_CROSS)

all: $(inprocess_test)

$(inprocess_test): $(OBJ_FILES)
	$(CXX) $(LD_PATHS) $(CC_FLAGS) -o $@ $^ $(LD_FLAGS)

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(inprocess_test) $(OBJ_FILES)
