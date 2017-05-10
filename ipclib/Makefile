ipc_lib := libipc.so
CC := gcc
CXX := g++
CPP_FILES := ipclib.cpp
OBJ_FILES := ipclib.o

CXX_FLAGS += -m32 -std=c++11
prefix := /usr/local

all: $(ipc_lib)

install: $(ipc_lib)
	install -m 0755 $(ipc_lib) -t $(prefix)/lib -D

$(ipc_lib): $(OBJ_FILES)
	$(CXX) $(CXX_FLAGS) -shared -o $@ $(OBJ_FILES)

%.o: %.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

.PHONY: clean
clean:
	$(RM) -r $(ipc_lib) $(OBJ_FILES)

