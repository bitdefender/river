lib_http_parser := libhttp-parser.so
CC := gcc
CXX := g++
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix ,$(notdir $(CPP_FILES:.cpp=.o)))
LD_FLAGS := -ldl
CC_LIB := -fPIC
CC_FLAGS += -m32 -std=c++11 -D_PAYLOAD_EXPORTS $(CC_FLAGS_CROSS)
prefix := /usr/local

all: $(lib_http_parser)

install: $(lib_http_parser)
	install -m 0755 $(lib_http_parser) -t $(prefix)/lib -D

$(lib_http_parser): $(OBJ_FILES)
	$(CXX) $(CC_FLAGS) $(LD_FLAGS) -shared -o $@ $^

%.o: %.cpp
	   $(CXX) $(CC_FLAGS) -c $(CC_LIB) -o $@ $<

.PHONY: clean
clean:
	$(RM) $(lib_http_parser) $(OBJ_FILES)
