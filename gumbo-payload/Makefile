
lib_gumbo := libgumbo.so
CC := gcc
CXX := g++
OBJ_FILES := ./build/.libs/libgumbo.a Payload.o
CC_FLAGS += -m32
CXX_FLAGS += -m32 -std=c++11 -D_PAYLOAD_EXPORTS
prefix := /usr/local

all: $(lib_gumbo)

install: $(lib_gumbo)
	install -m 0755 $(lib_gumbo) -t $(prefix)/lib -D

$(lib_gumbo): $(OBJ_FILES)
	$(CXX) $(CC_FLAGS) $(LD_FLAGS) -nostdlib -shared -o $@ $^

Payload.o: Payload.cpp
	   $(CXX) $(CXX_FLAGS) -I./gumbo-src/src -c $<

gumbo.o: gumbo.c
	   $(CC) $(CC_FLAGS) -c $<

.PHONY: clean
clean:
	$(RM) $(lib_gumbo) $(OBJ_FILES)
