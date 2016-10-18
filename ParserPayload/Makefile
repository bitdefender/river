lib_http_parser := libhttp-parser.so
CC := gcc
CXX := g++
OBJ_FILES := http_parser.o Payload.o
NO_SSE := -mno-mmx -mno-sse -march=i386
CC_FLAGS += -m32 $(NO_SSE)
CXX_FLAGS += -m32 -std=c++11 $(NO_SSE) -D_PAYLOAD_EXPORTS
prefix := /usr/local

all: $(lib_http_parser)

install: $(lib_http_parser)
	install -m 0755 $(lib_http_parser) -t $(prefix)/lib -D

$(lib_http_parser): $(OBJ_FILES)
	$(CXX) $(CC_FLAGS) $(LD_FLAGS) -shared -o $@ $^

Payload.o: Payload.cpp
	   $(CXX) $(CXX_FLAGS) -c $<

http_parser.o: http_parser.c
	   $(CC) $(CC_FLAGS) -c $<

.PHONY: clean
clean:
	$(RM) $(lib_http_parser) $(OBJ_FILES)
