lib_revwrapper := librevtracerwrapper.so
CC := gcc
CXX := g++
CPP_FILES := RevtracerWrapper.cpp Wrapper.Linux.cpp Wrapper.Windows.cpp
OBJ_FILES := $(addprefix ,$(notdir $(CPP_FILES:.cpp=.o)))
LD_FLAGS := -ldl
CC_FLAGS += -g -m32 -std=c++11
LIB_BINLOADER_PATH := ../BinLoader/libbinloader.a
prefix := /usr/local

all: $(lib_revwrapper)

test: $(lib_revwrapper) Main.o
	$(CXX) $(CC_FLAGS) Main.o -L. -lrevtracerwrapper -lrt -o test

install: $(lib_revwrapper)
	install -m 0755 $(lib_revwrapper) -t $(prefix)/lib -D

$(lib_revwrapper): $(OBJ_FILES)
	$(CXX) $(CC_FLAGS) -shared -o $@ $^ $(LIB_BINLOADER_PATH) $(LD_FLAGS)

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c $(CC_LIB) -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_revwrapper) $(OBJ_FILES) test Main.o
