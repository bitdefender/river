lib_revwrapper := rev-wrapper.so
CC := gcc
CXX := g++
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix ,$(notdir $(CPP_FILES:.cpp=.o)))
LD_FLAGS := -ldl
CC_FLAGS += -m32 -std=c++11 $(CC_FLAGS_CROSS)

all: $(lib_revwrapper)

$(lib_revwrapper): $(OBJ_FILES)
	$(CXX) $(CC_FLAGS) $(LD_FLAGS) -shared -o $@ $^

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_revwrapper) $(OBJ_FILES)
