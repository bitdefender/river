# https://software.intel.com/en-us/articles/performance-tools-for-software-developers-bsymbolic-can-cause-dangerous-side-effects

lib_http_parser := libhttp-parser.so
CC := gcc
CXX := g++
OBJ_FILES := http_parser.o Payload.o
DEFAULT_CFLAGS := -m32 -O2 -g
CC_FLAGS += $(DEFAULT_CFLAGS)
CXX_FLAGS += $(DEFAULT_CFLAGS) -nostdlib -std=c++11 -D_PAYLOAD_EXPORTS
prefix := /usr/local

all: $(lib_http_parser)

install: $(lib_http_parser)
	install -m 0755 $(lib_http_parser) -t $(prefix)/lib -D

$(lib_http_parser): $(OBJ_FILES)
	$(CXX) $(CC_FLAGS) -Wl,--version-script=http-parser.version  -shared -o $@ $^

Payload.o: Payload.cpp
	   $(CXX) $(CXX_FLAGS) -c $<

http_parser.o: http_parser.c
	   $(CC) $(CC_FLAGS) -c $<

.PHONY: clean
clean:
	$(RM) $(lib_http_parser) $(OBJ_FILES)
