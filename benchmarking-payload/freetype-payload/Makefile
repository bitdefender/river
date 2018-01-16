mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
makefile_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

freetype := libfreetype.so
CC := gcc
CXX := g++
INCLUDE_PATHS = "-I$(makefile_dir)/freetype-src/include -I$(makefile_dir)/freetype-src/"
OBJ_FILES := Payload.o $(makefile_dir)/freetype-src/objs/.libs/libfreetype.a
CXX_FLAGS += -m32 -std=c++11 -D_PAYLOAD_EXPORTS
prefix := /usr/local

all: $(freetype)

install: $(freetype)
	install -m 0755 $(freetype) -t $(prefix)/lib -D

$(freetype): $(OBJ_FILES)
	$(CXX) -m32 -shared $(OBJ_FILES) -o $@ -lz -larchive -lpng12

Payload.o: Payload.cpp
	   $(CXX) $(CXX_FLAGS) $(INCLUDE_PATHS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) $(freetype) Payload.o
