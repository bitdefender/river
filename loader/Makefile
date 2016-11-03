lib_loader := libloader.so
CC := gcc
CXX := g++
CPP_FILES := loader.linux.cpp
OBJ_FILES := loader.linux.o
CC_FLAGS += -g -m32 -march=i386 -std=c++11
prefix := /usr/local

all: $(lib_loader)

install: $(lib_loader)
	install -m 0755 $(lib_loader) -t $(prefix)/lib -D

$(lib_loader): $(OBJ_FILES)
	$(CXX) $(CC_FLAGS) -shared -o $@ $^

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_loader) $(OBJ_FILES)
