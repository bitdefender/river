lib_revwrapper := librevtracerwrapper.so
CC := gcc
CXX := g++
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix ,$(notdir $(CPP_FILES:.cpp=.o)))
LD_FLAGS := -ldl
CC_FLAGS += -m32 -std=c++11
prefix := /usr/local

all: $(lib_revwrapper)

install: $(lib_revwrapper)
	install -m 0755 $(lib_revwrapper) -t $(prefix)/lib -D

$(lib_revwrapper): $(OBJ_FILES)
	$(CXX) $(CC_FLAGS) -shared -o $@ $^ $(LD_FLAGS)

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c $(CC_LIB) -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_revwrapper) $(OBJ_FILES)